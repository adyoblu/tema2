#ifndef FILE_H
#define FILE_H
#include <stddef.h>

typedef struct File {
    char path[1024];
    struct File *next;
} File;

File* update_nth_node(File *head, int position, const char *new_path);
File* delete_nth_node(File *head, int position);
File* add_nth_node(File *head, const char *path_add, int position);
File* add_node_last(File *head, const char *path_add);
File* add_node_initial(File *head, const char *path_add);
char* choose_file(File *head);
void remove_all_nodes(File **head);
void print_files(File *head);

#endif