#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 9999
#define BUFFER_SIZE 4096

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    int *fd_ptr = (int *)arg;
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];

    printf("[*] New client connected!\n");

    while (1) {
        printf("Client [%d] > ", client_fd);
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = 0;

        if (strlen(command) == 0) continue;

        // Send command to client
        send(client_fd, command, strlen(command), 0);

        if (strcmp(command, "exit") == 0) break;

        // Receive output
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        
        if (bytes_received > 0) {
            printf("%s\n", buffer);
        } else {
            printf("[!] Client disconnected.\n");
            break;
        }
    }

    close(client_fd);
    free(fd_ptr);
    return NULL;
}

int main() {
    int server_fd, *new_sock;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("[*] Advanced Server listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

        if (client_fd < 0) continue;

        pthread_t thread_id;
        new_sock = malloc(sizeof(int));
        *new_sock = client_fd;

        if (pthread_create(&thread_id, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Thread creation failed");
            return 1;
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}