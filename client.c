#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <errno.h>

#define SERVER_PORT 8080 
#define BUFFER_SIZE 4096
#define LIST 1
#define DOWNLOAD 2
#define UPLOAD 3
#define DELETE 4
#define MOVE 5
#define UPDATE 6
#define SEARCH 7

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
        fprintf(stderr, "%d\n", userChoice);
        switch (userChoice) {
            case LIST://list
                sock = connectServer();
                sent = 0x0;
                send(sock, &sent, sizeof(sent), 0);
                fprintf(stderr, "%d\n", userChoice);
                break;
            case DOWNLOAD://download
                sock = connectServer();
                sent = 0x1;
                send(sock, &sent, sizeof(sent), 0);

                break;
            case UPLOAD://upload
                sock = connectServer();
                sent = 0x2;
                send(sock, &sent, sizeof(sent), 0);

                break;
            case DELETE://delete
                sock = connectServer();
                sent = 0x4;
                send(sock, &sent, sizeof(sent), 0);

                break;
            case MOVE://move
                sock = connectServer();
                sent = 0x8;
                send(sock, &sent, sizeof(sent), 0);

                break;
            case UPDATE://update
                sock = connectServer();
                sent = 0x10;
                send(sock, &sent, sizeof(sent), 0);

                break;
            case SEARCH://search
                sock = connectServer();
                sent = 0x20;
                send(sock, &sent, sizeof(sent), 0);

                break;
        }
        printf("\e[1;1H\e[2J");//clear screen
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