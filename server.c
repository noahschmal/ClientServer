//
// Created by Noah Schmalenberger on 10/15/24.
//

//#include "peer.h"
#include <unistd.h>     // sleep(3)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#define SIZE 1024

///////////////////////
/* Custom Data Types */
///////////////////////

typedef struct user {
    char name[20];
    char SERVER_IP[20];
    int port;
}user;

typedef struct connection {
    struct user* connected_user;
    struct connection* next_connection;
}connection;

void sending(user out, char* message);
void receiving(int server_fd, connection** list);
void *receive_thread(int s_fd, connection** list); //void *con);
void create_network(user* host, int* server_fd, connection** list);
void send_file(user out, char* filename);
void receive_file(connection** list, int server_fd);
void receive_message(int server_fd);
void receive_client(connection** list, int client_fd);


int main(int argc, char const* argv[])
{

    /////////////////////////////
    /* Gather User Information */
    /////////////////////////////

    user host;
    connection* connection_list = malloc(sizeof(connection));
    connection_list->connected_user = NULL;
    connection_list->next_connection = NULL;

    char hostbuffer[256];
    struct hostent *host_entry;
    int hostname;
    struct in_addr **addr_list;

    // Retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));

    // Retrieve IP addresses
    host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL) {
        herror("[-]gethostbyname error");
        exit(1);
    }
    addr_list = (struct in_addr **)host_entry->h_addr_list;

    // Print out the IP Addresses
    printf("IP address: \n");
    printf("[0] Enter custom address\n");
    for (int i = 0; addr_list[i] != NULL; i++) {
        printf("[%d] %s\n", i+1, inet_ntoa(*addr_list[i]));
    }

    // Have the user choose which ip address to use
    int ip_choice;
    printf("Enter your ip selection: ");
    scanf("%d", &ip_choice);
    if (ip_choice != 0) {
        strcpy(host.SERVER_IP, inet_ntoa(*addr_list[ip_choice - 1]));
    } else if (ip_choice == 0) {
        printf("Enter your IP: ");
        scanf("%s", host.SERVER_IP);
    }

    printf("Enter your port number: ");
    scanf("%d", &host.port);

    int server_fd;
    create_network(&host, &server_fd, &connection_list);

    // Server: close socket
    printf("[+]Closing the server connection.\n");
    close(server_fd);

    return 0;
}

void create_network(user* host, int* server_fd, connection** list) {
    ///////////////////////////////
    /* Create Server Information */
    ///////////////////////////////
    printf("[-+-]Server ip: %s:%d\n", host->SERVER_IP, host->port);

    // Server: Connection information
    int connectStatus;

    // Server: Socket and address
    struct sockaddr_in server_addr;

    // Server: Create socket
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(*server_fd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    // Server: Set address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = host->port;
    server_addr.sin_addr.s_addr = inet_addr(host->SERVER_IP); //INADDR_ANY;

    // Server: Bind socket
    connectStatus = bind(*server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(connectStatus < 0) {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successfull.\n");

    // Server: Listen with socket
    if (listen(*server_fd, 10) == 0) {
        printf("[+]Listening....\n");
    } else {
        perror("[-]Error in listening");
        exit(1);
    }

    receive_thread(*server_fd, list);
}

//Sending messages to port
void sending(user out, char* message)
{
    char *ip = out.SERVER_IP;
    int port = out.port;
    int connectStatus;

    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Client socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    connectStatus = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(connectStatus == -1) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server.\n");

    send(client_fd, message, 255, 0);

    printf("[+]Message sent.\n");
    printf("[+]Closing the client connection.\n");
    close(client_fd);
}

// Calling receiving every 2 seconds
void *receive_thread(int s_fd, connection** list)//void *con)
{

    while (1)
    {
        sleep(2);
        receiving(s_fd, list);
    }
}

//Receiving messages on our port
void receiving(int server_fd, connection** list)
{
    char strData[255];
    int client_socket = accept(server_fd, NULL, NULL);

    if (client_socket != -1) {
        printf("Receiving...\n");
        recv(client_socket, strData, sizeof(strData), 0);

        if (strcmp(strData, "FILE_TRANSFER") == 0) {
            printf("[+] Receiving File...\n");
            receive_file(list, client_socket);
        } else if (strcmp(strData, "CONNECTION_INFO") == 0)
        {
            //receive_client(list, client_socket);
        } else {

            printf("%s\n", strData);

            user test_user;
            strcpy(test_user.SERVER_IP, "68.234.244.147");
            test_user.port = 8081;
            strcpy(test_user.name, "Noah");
            sending(test_user, strData);

//            connection* head = *list;
//            while (head->connected_user != NULL) {
//                sending(*head->connected_user, strData);
//                head = head->next_connection;
//            }
        }
    }
}

void receive_file(connection** list, int client_fd) {
    int n;
    FILE *fp;
    char *filename = "recvserver.txt";
    char buffer[SIZE];

    fp = fopen(filename, "wb");
    while (1) {
        n = recv(client_fd, buffer, SIZE, 0);
        if (n <= 0){
            break;
            return;
        }

        fprintf(fp, "%s", buffer);
        bzero(buffer, SIZE); // memset()
    }
    fclose(fp);

//    connection* head = *list;
//    while (head->connected_user != NULL) {
//        send_file(*head->connected_user, filename);
//        head = head->next_connection;
//    }

    user test_user;
    strcpy(test_user.SERVER_IP, "68.234.244.147");
    test_user.port = 8081;
    strcpy(test_user.name, "Noah");
    //send_file(test_user, filename);
    return;
}

void send_file(user out, char* filename) {

    // file stuff and things
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("[-]Error in reading file");
        exit(1);
    }

    char hello[1024] = {0};

    char *ip = out.SERVER_IP;
    int port = out.port;
    int connectStatus;

    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Client socket created successfully\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    connectStatus = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(connectStatus == -1) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server\n");

    int n;
    char data[SIZE] = {0};
    char buffer[32] = "FILE_TRANSFER";

    send(client_fd, buffer, sizeof(buffer), 0);

    while(fgets(data, SIZE, fp) != NULL) {
        printf("sending: %s\n", data);
        if (send(client_fd, data, sizeof(data), 0) == -1) {
            perror("[-]Error in sending file\n");
            exit(1);
        }
        bzero(data, SIZE);
    }

    printf("[+]File sent\n");
    printf("[+]Closing the client connection\n");
    close(client_fd);
}

void receive_client(connection** list, int client_fd)
{
    connection head;
    head.connected_user = malloc(sizeof(user));
    head.next_connection = NULL;

    sleep(1);

    char buffer[20];
    recv(client_fd, buffer, sizeof(buffer), 0); // NAME
    strcpy(head.connected_user->name, buffer);

    strcpy(head.connected_user->name, "Noah");

    bzero(buffer, sizeof(buffer)); // memset()

    recv(client_fd, buffer, sizeof(buffer), 0); // IP
    strcpy(head.connected_user->SERVER_IP, buffer);

    strcpy(head.connected_user->SERVER_IP, "68.234.244.147");

    bzero(buffer, sizeof(buffer)); // memset()

    recv(client_fd, buffer, sizeof(buffer), 0); // PORT
    head.connected_user->port = atoi(buffer);

    head.connected_user->port = 8081;

    bzero(buffer, sizeof(buffer)); // memset()

    connection* it = *list;
    if (it->connected_user == NULL) {
        *list = &head;
        printf("new head\n");
    } else {
        printf("existing head\n");
        while (it->next_connection != NULL) {
            it = it->next_connection;
        }
        it->next_connection = &head;
    }
}