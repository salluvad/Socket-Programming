#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define SPHERE_PORT 7777

// client handler API
void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while (1) {
        bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_read == -1) {
            perror("recv");
            exit(1);
        } else if (bytes_read == 0) {
            printf("Connection closed by client\n");
            close(client_fd);
            return;
        }
        printf("Received %ld bytes from client: %.*s\n", bytes_read, (int) bytes_read, buffer);
        if (send(client_fd, buffer, bytes_read, 0) == -1) {
            perror("send");
            exit(1);
        }
        printf("Sent  %ld bytes to client: %.*s\n", bytes_read, (int) bytes_read, buffer);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SPHERE_PORT);   // Listening on server port 7777
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        exit(1);
    }
    printf("Listening on port 7777...\n");

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept");
            exit(1);
        }
        printf("Accepted connection from client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        if (fork() == 0) {     // child proc.
            handle_client(client_fd);
          exit(0);
       }
        close(client_fd);
    }
    return 0;
}

