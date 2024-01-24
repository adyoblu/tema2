#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <errno.h>
#include <libgen.h>
#include "file.h"

#define SERVER_PORT 8080 
#define BUFFER_SIZE 4096
#define LIST 1
#define DOWNLOAD 2
#define UPLOAD 3
#define DELETE 4
#define MOVE 5
#define UPDATE 6
#define SEARCH 7

#define SUCCESS 0x0
#define FILE_NOT_FOUND 0x1
#define PERMISSION_DENIED 0x2
#define OUT_OF_MEMORY 0x4
#define SERVER_BUSY 0x8
#define UNKNOWN_OPERATION 0x10
#define BAD_ARGUMENTS 0x20
#define OTHER_ERROR 0x40
File *file_list = NULL;
int connectServer();

void print_menu()
{   
    printf("\n###############\n");
    printf("1. LIST       #\n");
    printf("2. DOWNLOAD   #\n");
    printf("3. UPLOAD     #\n");
    printf("4. DELETE     #\n");
    printf("5. MOVE       #\n");
    printf("6. UPDATE     #\n");
    printf("7. SEARCH     #\n");
    printf("###############\n");
	printf("Introduceti optiunea dorita:\n");
    fflush(stdout);
}

void receive_list(int sock) {
    remove_all_nodes(&file_list);
    int32_t status;
    recv(sock, &status, sizeof(status), 0);

    if (status == SUCCESS) {
        //number of bytes
        size_t num_bytes;
        recv(sock, &num_bytes, sizeof(num_bytes), 0);

        //file content
        char *buffer = (char*) malloc(sizeof(char)*num_bytes);
        ssize_t total_received = 0;
        ssize_t bytes_received;

        while (total_received < num_bytes) {
            bytes_received = recv(sock, buffer + total_received, num_bytes - total_received, 0);
            if (bytes_received <= 0) {
                perror("recv");
                exit(EXIT_FAILURE);
            }
            total_received += bytes_received;
        }

        size_t i = 0;
        while (i < num_bytes) {
            file_list = add_node_last(file_list, buffer + i);
            i += strlen(buffer + i) + 1;
        }

        free(buffer);
    } else {
        printf("Error: Server failed to send the list.\n");
    }
}

void press_any_key_to_continue() {
    printf("\nPress Enter to continue...");
    while (getchar() != '\n') {
    }
    getchar();
}

void recv_files(int sock, int sent){
    receive_list(sock);
}

void download_file(int sock, int sent){
    size_t len;
    char buffer[1024];
    strcpy(buffer, choose_file(file_list));
    //fprintf(stderr, "%s\n", buffer);
    len = strlen(buffer);
    send(sock, &len, sizeof(len), 0);//dim path
    send(sock, buffer, len, 0);//path

    int32_t status;
    recv(sock, &status, sizeof(status), 0);
    
    // Receive the file size
    off_t file_size;
    recv(sock, &file_size, sizeof(file_size), 0);
    char* filename = basename(buffer);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Error opening or creating file");
        exit(EXIT_FAILURE);
    }

    // Receive the file content
    char buffer_content[1024];
    ssize_t total_received = 0;
    ssize_t bytes_received;

    while (total_received < file_size) {
        bytes_received = recv(sock, buffer_content, sizeof(buffer_content), 0);
        if (bytes_received <= 0) {
            perror("recv");
            close(fd);
            unlink(filename);
            exit(EXIT_FAILURE);
        }

        write(fd, buffer_content, bytes_received);
        total_received += bytes_received;
    }

    close(fd);

    printf("Download successful. Salvat ca : %s\n", buffer);
    press_any_key_to_continue();
}

void handleServerActions() {
    int32_t sent, userChoice;
    int sock;
    while(1){
        print_menu();
        do {
        printf("Optiunea (1-7): ");
            if (scanf("%d", &userChoice) != 1 || userChoice < 1 || userChoice > 7) {
                printf("Invalid input. Introdu intre 1 si 7!\n");
                while (getchar() != '\n');
            }
        } while (userChoice < 1 || userChoice > 7);
        printf("\n\n");
        //fprintf(stderr, "%d\n", userChoice);
        switch (userChoice) {
            case LIST://list
                sent = 0x0;
                sock = connectServer();
                send(sock, &sent, sizeof(sent), 0);
                recv_files(sock, sent);
                print_files(file_list);
                press_any_key_to_continue();
                break;
            case DOWNLOAD://download
                sent = 0x1;
                sock = connectServer();
                send(sock, &sent, sizeof(sent), 0);
                recv_files(sock, sent);
                download_file(sock, sent);
                break;
            case UPLOAD://upload
                sock = connectServer();
                sent = 0x2;//IN: cod_operație;
                send(sock, &sent, sizeof(sent), 0);
                //nr. octeți cale fișier; 
                //calea fișierului; 
                //nr. octeți conținut; 
                //conținut fișier
                break;
            case DELETE://delete
                sock = connectServer();
                sent = 0x4;// IN: cod_operație;
                send(sock, &sent, sizeof(sent), 0);
                // nr. octeți cale fișier; 
                // calea fișierului

                break;
            case MOVE://move
                sock = connectServer();
                sent = 0x8;//IN: cod_operație; 
                send(sock, &sent, sizeof(sent), 0);
                //nr_octeți_cale_fișier_sursă; 
                //cale_fișier_sursă_\0; 
                //nr_octeți_cale_fișier_destinație; 
                //cale_fișier_desitnație_\0

                break;
            case UPDATE://update
                sock = connectServer();
                sent = 0x10;//IN: cod_operație; 
                //nr. octeți cale fișier; 
                //calea fișierului; 
                //octet start; 
                //dimensiune; caracterele noi

                send(sock, &sent, sizeof(sent), 0);

                break;
            case SEARCH://search
                sock = connectServer();
                sent = 0x20;//IN: cod_operație;
                send(sock, &sent, sizeof(sent), 0);
                //nr. octeți cuvânt; 
                //cuvânt 
                break;
        }
        //printf("\e[1;1H\e[2J");//clear screen
    }
}

int connectServer(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        fprintf(stderr, "Eroare la conectarea la server!\n");
        exit(EXIT_FAILURE);
    }
    return sock;
}

int main(int argc, char *argv[]) {
    connectServer();
    handleServerActions();
    return 0;
}