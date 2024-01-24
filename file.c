#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#include <string.h>

File* delete_nth_node(File *head, int position) {
    File *temp1 = head;

    if (position == 1) {
        head = temp1->next;
        free(temp1);
        return head;
    }

    for (int i = 0; i < position - 2; ++i) {
        temp1 = temp1->next;  // (position-1)th node
    }

    File *temp2 = temp1->next;  // position-th node
    temp1->next = temp2->next;
    free(temp2);
    return head;
}

void remove_all_nodes(File **head) {
    File *current = *head;
    File *next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    // Update the head pointer to NULL
    *head = NULL;
}

File* add_nth_node(File *head, const char *path_add, int position) {
    File *temp1 = (File*)malloc(sizeof(File));
    strcpy(temp1->path, path_add);

    File *temp2 = head;
    if (position == 1) {
        temp1->next = head;
        head = temp1;
        return head;
    }

    for (int i = 0; i < position - 2; ++i) {
        temp2 = temp2->next;
    }

    temp1->next = temp2->next;
    temp2->next = temp1;
    return head;
}

File* add_node_last(File *head, const char *path_add) {
    File *temp = (File*)malloc(sizeof(File));
    if (temp == NULL) {
        return NULL;
    }
    strcpy(temp->path, path_add);
    temp->next = NULL;

    if (head == NULL) {
        return temp;
    }

    File *temp1 = head;
    while (temp1->next != NULL) {
        temp1 = temp1->next;
    }

    temp1->next = temp;
    return head;
}

File* update_nth_node(File *head, int position, const char *new_path) {
    File *temp1 = head;

    for (int i = 0; i < position - 1; ++i) {
        if (temp1 == NULL) {
            printf("Error: Node at position %d does not exist.\n", position);
            return head;
        }
        temp1 = temp1->next;
    }

    if (temp1 != NULL) {
        strcpy(temp1->path, new_path);
    }

    return head;
}

File* add_node_initial(File *head, const char *path_add) {
    File *temp = (File*)malloc(sizeof(File));
    strcpy(temp->path, path_add);
    temp->next = head;
    head = temp;
    return head;
}

void print_files(File *head) {
    File *temp1 = head;
    while (temp1 != NULL) {
        printf("%s\n", temp1->path);
        temp1 = temp1->next;
    }
}

char* choose_file(File *head) {
    File *temp1 = head;
    int order = 1;

    printf("Alege un path:\n");
    while (temp1 != NULL) {
        printf("%d) %s\n", order++, temp1->path);
        temp1 = temp1->next;
    }

    int choice;
    char *chosen_path;

    do {
        printf("Introdu numarul: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input.\n");
            while (getchar() != '\n');
            continue;
        }

        if (choice >= 1 && choice <= order) {
            temp1 = head;

            while (temp1 != NULL && --choice > 0) {
                temp1 = temp1->next;
            }

            chosen_path = strdup(temp1->path);
            return chosen_path;
        } else {
            printf("Invalid input. numar intre 1 si %d.\n", order - 1);
        }
    } while (1); 
}

// int main() {
//     File *file_list = NULL;

//     // Example usage
//     file_list = add_node_last(file_list, "/file1");
//     file_list = add_node_last(file_list, "/file2");
//     file_list = add_node_last(file_list, "/file3");

//     printf("Original list:\n");
//     print_files(file_list);

//     file_list = add_node_initial(file_list, "/file0");
//     printf("\nList after adding at the beginning:\n");
//     print_files(file_list);

//     file_list = add_nth_node(file_list, "/file1.5", 2);
//     printf("\nList after adding at position 2:\n");
//     print_files(file_list);

//     file_list = delete_nth_node(file_list, 3);
//     printf("\nList after deleting at position 3:\n");
//     print_files(file_list);

//     file_list = update_nth_node(file_list, 2, "/updated_file/1.5");
//     printf("\nList after updating the name at position 2:\n");
//     print_files(file_list);

//     return 0;
// }
