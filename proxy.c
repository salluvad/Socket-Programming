#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BOX1_PORT 8888
#define BOX2_PORT 8889
#define BOX3_PORT 8890
#define SPHERE_PORT 7777
#define BUFFER_SIZE 1024

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);

    // buffer for reading from client

    char client_buffer[1024];
    int client_bytes_read = 0;

    // buffer for reading from upstream server
    char upstream_buffer[1024];
    int upstream_bytes_read = 0;

    // Connect to upstream server
    int upstream_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in upstream_address;
    memset(&upstream_address, 0, sizeof(upstream_address));
    upstream_address.sin_family = AF_INET;
    upstream_address.sin_addr.s_addr = INADDR_ANY;
    upstream_address.sin_port = htons(SPHERE_PORT);

    if (connect(upstream_fd, (struct sockaddr *)&upstream_address, sizeof(upstream_address)) < 0) {
        perror("connect failed");
        close(client_fd);
        close(upstream_fd);
        return NULL;
    }
    
    // Forward data between client and upstream server

    while (1) {
        // read from client
        client_bytes_read = recv(client_fd, client_buffer, sizeof(client_buffer), 0);
        if (client_bytes_read <=0-1 && (errno == EAGAIN || errno == EWOULDBLOCK)) { // to continue listening need to check the error codes as well
            // no data available on client socket, continue
            continue;
        } else if (client_bytes_read <= 0) {
            perror( "Error or client closed connection");
            close(client_fd);
            close(upstream_fd);
            return NULL;
        } else {
            printf("read %ld bytes from client\n",client_bytes_read);
        }

        // send data to upstream server

        if (send(upstream_fd, client_buffer, client_bytes_read, 0) < 0) {
            perror("send failed to sphere");
            close(client_fd);
            close(upstream_fd);
            return NULL;
        }

        // read from upstream server
        upstream_bytes_read = recv(upstream_fd, upstream_buffer, sizeof(upstream_buffer), 0);

        if (upstream_bytes_read <=0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // no data available on upstream server socket, continue
            printf("continue no data from upstream");
            continue;
        } else if (upstream_bytes_read <= 0) {
            // error or upstream server close connection
           close(client_fd);
           close(upstream_fd);
           return NULL;
       } else {
            printf("read %ld bytes from server  \n",upstream_bytes_read);
       }
        
        // send data back to client
        if (send(client_fd, upstream_buffer, upstream_bytes_read, 0) < 0) {
            perror("send failed to client");
            close(client_fd);
            close(upstream_fd);
            return NULL;
        } 
          printf("sent data back to client %s",upstream_buffer);
    }
   pthread_exit();
}



int main(int argc, char *argv[]) {
    int server_fd1, server_fd2, server_fd3, sphere_fd, addrlen;
    struct sockaddr_in box1_address, box2_address, box3_address, sphere_address;
    pthread_t threads[4];

    // Create a socket for Box 1

    if ((server_fd1 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to  Box 1 port
    box1_address.sin_family = AF_INET;
    //box1_address.sin_addr.s_addr = inet_addr("192.168.0.1");
    box1_address.sin_addr.s_addr = INADDR_ANY;
    box1_address.sin_port = htons(BOX1_PORT);

    if (bind(server_fd1, (struct sockaddr *)&box1_address, sizeof(box1_address)) < 0) {
        printf("1");
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections from Box 1
    if (listen(server_fd1, 1) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Create a socket for Box 2
    if ((server_fd2 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // bind the socket to  Box 2 port
    box2_address.sin_family = AF_INET;
    //box2_address.sin_addr.s_addr = inet_addr("192.168.0.1");
    box2_address.sin_addr.s_addr = INADDR_ANY;
    box2_address.sin_port = htons(BOX2_PORT);

    if (bind(server_fd2, (struct sockaddr *)&box2_address, sizeof(box2_address)) < 0) {
        printf("2");
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections from Box 2
    if (listen(server_fd2, 1) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    
    // Create a socket for Box 3
    if ((server_fd3 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Bind the socket to  Box 3 port

    box3_address.sin_family = AF_INET;
    //box3_address.sin_addr.s_addr = inet_addr("192.168.0.1");
    box3_address.sin_addr.s_addr = INADDR_ANY;
    box3_address.sin_port = htons(BOX3_PORT);

    if (bind(server_fd3, (struct sockaddr *)&box3_address, sizeof(box3_address)) < 0) {
        printf("3");
        perror("bind failed");
        exit(EXIT_FAILURE);
    }


    // Listen for incoming connections from Box 3

    if (listen(server_fd3, 1) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Create a socket for the Sphere
    if ((sphere_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections from Box 1, Box 2, and Box 3
    while (1) {
        int client_fd1, client_fd2, client_fd3;
        struct sockaddr_in client_address1;
        struct sockaddr_in client_address2;
        struct sockaddr_in client_address3;
        addrlen = sizeof(client_address1);

        // accept connection from Box 1

        if ((client_fd1 = accept(server_fd1, (struct sockaddr *)&client_address1, (socklen_t *)&addrlen)) >= 0) {
            int *arg = malloc(sizeof(*arg));
            *arg = client_fd1;
            printf("conection accepted from client 1 \n");       
            pthread_create(&threads[0], NULL, handle_client, arg);
        }

        // accept connection from Box 2
        if ((client_fd2 = accept(server_fd2, (struct sockaddr *)&client_address2, (socklen_t *)&addrlen)) >= 0) {
            int *arg = malloc(sizeof(*arg));
            *arg = client_fd2;
            printf("conection accepted from client 3");           
            pthread_create(&threads[1], NULL, handle_client, arg);
        }

        // accept connection from Box 3
        if ((client_fd3 = accept(server_fd3, (struct sockaddr *)&client_address3, (socklen_t *)&addrlen)) >= 0) {
            int *arg = malloc(sizeof(*arg));
            *arg = client_fd3;
            printf("conection accepted from client 2");            
            pthread_create(&threads[2], NULL, handle_client, arg);
        }
    }

    int i ;
    for (i = 0; i < 3; i++) {
    if (pthread_join(threads[i], NULL) == -1) {
        perror("pthread_join");
        exit(1);
    }
}

// Close server sockets

close(server_fd1);
close(server_fd2);
close(server_fd3);

return 0;
}


