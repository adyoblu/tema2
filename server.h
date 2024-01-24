#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <pthread.h>
#include <sys/epoll.h>

typedef struct File {
    char path[256];
    uint32_t size;
    struct File *next;
} File;

typedef struct {
    int sockfd;
    struct epoll_event event;
    pthread_t thread_id;
} ThreadInfo;

// void *server_thread(void *arg);
// void *client_handler(void *arg);
// void handle_list(ThreadInfo *client_data);
// void handle_download(ThreadInfo *client_data);
// void handle_upload(ThreadInfo *client_data);
// void handle_delete(ThreadInfo *client_data);
// void handle_move(ThreadInfo *client_data);
// void handle_update(ThreadInfo *client_data);
// void handle_search(ThreadInfo *client_data);
// void log_operation(const char *operation, const char *file_path, const char *search_term);

#endif