// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>

// #define PORT 8080

// int main() {
//     int sock;
//     struct sockaddr_in server_addr;
//     char buffer[1024] = "Hello Server";

//     sock = socket(AF_INET, SOCK_STREAM, 0);
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(PORT);
//     inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

//     connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
//     send(sock, buffer, strlen(buffer), 0);
//     read(sock, buffer, sizeof(buffer));
//     printf("Server response: %s\n", buffer);
//     close(sock);
//     return 0;
// }


// client.c  — interactive echo client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080

int main(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) != 1) {
        perror("inet_pton");
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    char sendbuf[1024];
    char recvbuf[2048];

    for (;;) {
        printf("> ");
        fflush(stdout);

        if (!fgets(sendbuf, sizeof(sendbuf), stdin)) break;      // EOF (Ctrl+D)
        size_t len = strcspn(sendbuf, "\r\n");                   // strip newline
        sendbuf[len] = '\0';

        if (send(sock, sendbuf, len, 0) < 0) { perror("send"); break; }

        if (strcmp(sendbuf, "exit") == 0) break;                 // tell server to close

        ssize_t n = read(sock, recvbuf, sizeof(recvbuf) - 1);
        if (n <= 0) { if (n < 0) perror("read"); break; }
        recvbuf[n] = '\0';
        printf("%s\n", recvbuf);                                 // e.g., "Echo: <msg>"
    }

    close(sock);
    return 0;
}