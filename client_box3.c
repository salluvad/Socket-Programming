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

int main() {
    int proxy_fd;
    struct sockaddr_in proxy_addr;
    char buffer[BUFFER_SIZE];
    ssize_t num_read;
    proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_fd == -1) {
        perror("socket");
        exit(1);
    }
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(8890);
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    if (connect(proxy_fd, (struct sockaddr*) &proxy_addr, sizeof(proxy_addr)) == -1) {
        perror("connect");
        exit(1);
    }
    printf("Connected to proxy server\n");
    while (1) {
        printf("Enter message to send: ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        if (send(proxy_fd, buffer, strlen(buffer), 0) == -1) {
            perror("send");
            exit(1);
        }
        num_read = recv(proxy_fd, buffer, BUFFER_SIZE, 0);
        if (num_read == -1) {
            perror("recv");
            exit(1);
        } else if (num_read == 0) {
            printf("Connection closed by server\n");
            close(proxy_fd);
            return 0;
        }
        printf("Received %ld bytes from server: %.*s\n", num_read, (int) num_read, buffer);
    }
    close(proxy_fd);
    return 0;
}

