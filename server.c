#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <dirent.h>
#include <sys/sendfile.h>

#define ROOT_DIRECTORY "root_dir"
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define LIST 0x0
#define DOWNLOAD 0x1
#define UPLOAD 0x2
#define DELETE 0x4
#define MOVE 0x8
#define UPDATE 0x10
#define SEARCH 0x20
#define MAX_EVENTS 3

#define SUCCESS 0x0
#define FILE_NOT_FOUND 0x1
#define PERMISSION_DENIED 0x2
#define OUT_OF_MEMORY 0x4
#define SERVER_BUSY 0x8
#define UNKNOWN_OPERATION 0x10
#define BAD_ARGUMENTS 0x20
#define OTHER_ERROR 0x40

uint32_t status;
static pthread_mutex_t file_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_key_t thread_info_key;
int epoll_fd;
int signal_fd;
int epollfd;
int server_sockfd;
volatile sig_atomic_t graceful = 0;
File *file_list = NULL;

typedef struct {
    int sockfd;
    struct epoll_event event;
    pthread_t thread_id;
} ThreadInfo;

void cleanup_thread_info(void *data) {
    free(data);
}

void init_thread_info() {
    pthread_key_create(&thread_info_key, cleanup_thread_info);
}

ThreadInfo* alloc_thread_info(int client_sockfd, ThreadInfo *thread_info, int epoll_event_flags) {
    ThreadInfo *info = (ThreadInfo*)pthread_getspecific(thread_info_key);
    if (info == NULL) {
        info = (ThreadInfo*)malloc(sizeof(ThreadInfo));
        info->sockfd = client_sockfd;
        info->event.data.ptr = thread_info;
        info->event.events = EPOLLIN;
        pthread_setspecific(thread_info_key, info);
    }
    return info;
}

void load_files_recursive(const char *directory, File **head) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(directory)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                // Skip "." and ".." entries for directories
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    char subdirectory[1024];
                    snprintf(subdirectory, sizeof(subdirectory), "%s/%s", directory, entry->d_name);
                    load_files_recursive(subdirectory, head);
                }
            } else {
                char file_path[1024];
                snprintf(file_path, sizeof(file_path), "%s/%s", directory, entry->d_name);

                // Remove the root_directory prefix
                const char *relative_path = file_path + strlen(ROOT_DIRECTORY);
                if (relative_path[0] == '/') {
                    relative_path++;  // Skip the leading '/'
                }

                // Add the file to the linked list
                *head = add_node_last(*head, relative_path);
            }
        }
        closedir(dir);
    } else {
        perror("Error opening directory");
    }
}

void load_files(const char *directory) {
    load_files_recursive(directory, &file_list);
    //print_files(file_list);
}

void send_list(int sock) {
    int32_t sent = SUCCESS;
    send(sock, &sent, sizeof(sent), 0);  // status
    int temp_fd = open("/tmp/temp_file", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    File *head = file_list;
    while (head != NULL) {
        write(temp_fd, head->path, strlen(head->path));
        write(temp_fd, "\0", 1);
        head = head->next;
    }

    off_t file_size = lseek(temp_fd, 0, SEEK_END);

    //număr octeți răspuns
    send(sock, &file_size, sizeof(file_size), 0);

    lseek(temp_fd, 0, SEEK_SET);

    //lista de fișiere, separate prin ‘\0’
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(temp_fd, buffer, sizeof(buffer))) > 0)
        send(sock, buffer, bytes_read, 0);

    close(temp_fd);
    unlink("/tmp/temp_file");
}


void download(int sock) {
    size_t bytes;
    char buffer[1024];
    //IN
    recv(sock, &bytes, sizeof(bytes), 0); // numar octeti cale fisier
    recv(sock, buffer, bytes, 0); // cale fisier
    //OUT
    //status; 
    int32_t sent = SUCCESS;
    send(sock, &sent, sizeof(sent), 0);  // status

    char result[1024];
    strcpy(result, ROOT_DIRECTORY);
    strcat(result, "/");
    strcat(result, buffer);

    int file_fd = open(result, O_RDONLY);
    if (file_fd == -1) {
        perror("Error opening file");
        return;
    }

    struct stat stat_buf;
    fstat(file_fd, &stat_buf);

    off_t file_size = stat_buf.st_size;
    send(sock, &file_size, sizeof(file_size), 0);
    off_t offset = 0;
    ssize_t sent_bytes = sendfile(sock, file_fd, &offset, file_size);

    if (sent_bytes == -1) {
        perror("Error sending file");
    }
    close(file_fd);
}

void *client_handler(void *arg) {
    init_thread_info();
    ThreadInfo *thread_info = (ThreadInfo*)arg;
    int client_sockfd = thread_info->sockfd;
    uint32_t operation;
    size_t len;
    ssize_t bytes_received = recv(client_sockfd, &operation, sizeof(operation), 0);
    if (bytes_received < 0) {
        fprintf(stderr, "Error receiving data from client\n");
        close(client_sockfd);
        return NULL;
    } else if (bytes_received == 0) {
        close(client_sockfd);
        return NULL;
    }

    fprintf(stderr, "0x%x\n", operation);

    switch (operation) {
        case LIST:
            send_list(client_sockfd);
            break;
        case DOWNLOAD:
            send_list(client_sockfd);
            download(client_sockfd);
            break;
        case UPLOAD:
            //OUT: status
            break;
        case DELETE:
            //OUT: status
            break;
        case MOVE:
            //OUT: status
            break;
        case UPDATE:
            //
            break;
        case SEARCH:
            //OUT: status; 
            //nr_octeti_dimensiune_lista; 
            //listă de fișiere separate prin '\0'
            break;
    }
    close(client_sockfd);
    if(graceful != 0) {
        close(server_sockfd);
        exit(0);
    }
    return NULL;
}

int initialise_server_sock(int n){
    struct sockaddr_in server_addr;
    int server_sockfd;
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8080);

    int opt = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sockfd, n) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    return server_sockfd;
}

void *signal_handler_thread(void *arg) {
    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == signal_fd) {
                struct signalfd_siginfo si;
                ssize_t size = read(signal_fd, &si, sizeof(si));
                if (size != sizeof(si)) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }

                if (si.ssi_signo == SIGINT || si.ssi_signo == SIGTERM) {
                    printf("\nGraceful termination.\n");
                    graceful = 1;
                }
            }   
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == fileno(stdin)) {
                char buffer[256];
                if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                    if (strcmp(buffer, "quit\n") == 0) {
                        printf("Graceful termination.\n");
                        graceful = 1;
                    }
                }
            }
        }
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    load_files(ROOT_DIRECTORY);
    int n;
    if(argc >= 2) 
        n = atoi(argv[1]);
    else 
        n = MAX_CLIENTS;
    int client_sockfd;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    int epoll_fd, epoll_fd_signal;
    struct epoll_event events[n];
    ThreadInfo *thread_info;
    server_sockfd = initialise_server_sock(n);

    sigset_t set;
    //adaug semnale in set
    if (sigemptyset(&set) != 0 || sigaddset(&set, SIGINT) != 0 || sigaddset(&set, SIGTERM) != 0) {
        perror("sigemptyset/sigaddset");
        exit(EXIT_FAILURE);
    }

    //blochez semnalele in thread-ul principal
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("pthread_sigmask");
        exit(EXIT_FAILURE);
    }

    signal_fd = signalfd(-1, &set, 0);
    if (signal_fd == -1) {
        perror("signalfd");
        exit(EXIT_FAILURE);
    }
    //fd pentru epoll
    epollfd = epoll_create1(0);
    struct epoll_event event2;
    event2.events = EPOLLIN;
    event2.data.fd = signal_fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, signal_fd, &event2);

    // Add stdin to epoll
    struct epoll_event event_stdin;
    event_stdin.events = EPOLLIN;
    event_stdin.data.fd = fileno(stdin);
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fileno(stdin), &event_stdin);

    //thread semnale
    pthread_t signal_thread;
    pthread_create(&signal_thread, NULL, signal_handler_thread, NULL);

    epoll_fd = epoll_create(n);
    struct epoll_event event;
    event.data.ptr = NULL;
    event.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sockfd, &event);

    while (!graceful) {
        int nfds = epoll_wait(epoll_fd, events, n, -1);
        if (nfds == -1 && errno != EINTR) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.ptr == NULL && events[i].events & EPOLLIN) {
                memset(&client_addr, 0, sizeof(client_addr));
                client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_len);
                if (client_sockfd == -1) {
                    perror("accept");
                    continue;
                }
                thread_info = alloc_thread_info(client_sockfd, thread_info, EPOLLIN);
                
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sockfd, &thread_info->event);
                pthread_create(&thread_info->thread_id, NULL, client_handler, (void*)thread_info);
            }
            if(graceful == 1){
                pthread_join(thread_info->thread_id, NULL);
                exit(EXIT_SUCCESS);
            }
        }
    }

    pthread_join(signal_thread, NULL);
    close(epollfd);
    close(signal_fd);

    return 0;
}