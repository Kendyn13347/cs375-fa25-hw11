// server.c — fork-per-connection with 10s idle timeout via select()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 8080
#define TIMEOUT_SEC 10

static void handle_client(int client_sock) {
    char buf[1024];

    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(client_sock, &rfds);

        struct timeval tv;
        tv.tv_sec  = TIMEOUT_SEC;
        tv.tv_usec = 0;

        int r = select(client_sock + 1, &rfds, NULL, NULL, &tv);
        if (r == 0) {
            // idle timeout
            fprintf(stderr, "client idle > %ds — closing\n", TIMEOUT_SEC);
            break;
        } else if (r < 0) {
            if (errno == EINTR) continue;   // interrupted by signal; retry
            perror("select");
            break;
        }

        // data available
        ssize_t n = read(client_sock, buf, sizeof(buf) - 1);
        if (n <= 0) break;                  // client closed or error
        buf[n] = '\0';

        // trim CR/LF (lets "exit\n" work nicely)
        while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';

        if (strcmp(buf, "exit") == 0) break;

        char reply[1200];
        int len = snprintf(reply, sizeof(reply), "Echo: %s", buf);
        if (len > 0) (void)write(client_sock, reply, (size_t)len);
    }

    close(client_sock);
}

int main(void) {
    // reap children automatically (avoid zombies)
    signal(SIGCHLD, SIG_IGN);

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) { perror("socket"); exit(1); }

    int yes = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(server_sock, 5) < 0) {
        perror("listen"); exit(1);
    }

    fprintf(stderr, "server listening on %d (child timeout %ds)\n", PORT, TIMEOUT_SEC);

    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) { perror("accept"); continue; }

        pid_t pid = fork();
        if (pid == 0) {
            // child
            close(server_sock);
            handle_client(client_sock);
            _exit(0);
        } else if (pid > 0) {
            // parent
            close(client_sock);
        } else {
            perror("fork");
            close(client_sock);
        }
    }
}
