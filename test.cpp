#include <cstring>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <wait.h>

#define UNUSED(__x__) (void)__x__

// server/daemon code from: https://gist.github.com/tscho/397539/d7fae241b8f6c3b1433ebc598a9168affc4b737d
// client code from: https://github.com/denehs/unix-domain-socket-example/blob/master/client.c

#define SOCK_PATH "/home/pi/projects/upnpd/my_echo_socket"
//#define LOG_PATH "/home/pi/projects/upnpd/upnpd_server.log"

void handle_server_connection(int);
int handle_args(int argc, char *argv[]);
int daemonize();
void stop_server(int);

int init_socket();

int write_to_socket();

int daemon_run = true;

int main(int argc, char *argv[]) {

    // handle args
    // TODO
    if (argc > 1) {
        return handle_args(argc, argv);
    } else {
        return daemonize();
    }
}

// TODO: non-blocking socket and use select()
int daemonize() {
    printf("Running as daemon...\n");
    fflush(stdout);

    int client_sockfd;
    struct sockaddr_un remote;
    socklen_t t;

    signal(SIGTERM, stop_server);

    int server_sockfd = init_socket();
    if (server_sockfd == -1) {
        return 1;
    }
    // end init_socket();

    printf("Listening...\n");
    fflush(stdout);

    if (listen(server_sockfd, 5) == -1) {
        perror("listen");
        return 1;
    }

    while (daemon_run) {
        printf("Waiting for a connection\n");
        fflush(stdout);

        t = sizeof(remote);
        if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&remote, &t)) == -1) {
            perror("accept");
            break;
        }

        printf("Accepted connection\n");
        fflush(stdout);

        handle_server_connection(client_sockfd);
    }
    if (!daemon_run) {
        printf("daemon_run == false\n");
    }
    printf("Daemon exiting...\n");
    fflush(stdout);

    close(server_sockfd);
    // unlink(PID_PATH); // Get rid of pesky pidfile, socket
    unlink(SOCK_PATH);

    return 0;
}

int init_socket() {
    struct sockaddr_un local;
    // int len;

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating server socket");
        return -1;
    }

    /*
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(sockfd, (struct sockaddr *)&local, len) == -1) {
        perror("binding");
        return -1;
    }*/

    // std::memset(&local, 0, sizeof(local));
    memset(&local, 0, sizeof(local));
    local.sun_family = AF_UNIX;
    std::strncpy(local.sun_path, SOCK_PATH, sizeof(local.sun_path) - 1);

    int bindResult = bind(sockfd, reinterpret_cast<sockaddr *>(&local), sizeof(local));
    if (bindResult == -1) {
        // std::cerr << "Failed to bind socket." << std::endl;
        perror("binding");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void handle_server_connection(int client_sockfd) {
    char buff[1024];
    unsigned int len;

    printf("Handling connection\n");
    fflush(stdout);

    while (len = recv(client_sockfd, &buff, 1024, 0), len > 0) {
        printf("Received len %d: %s\n", len, buff);
        send(client_sockfd, &buff, len, 0);
    }

    close(client_sockfd);
    printf("Done handling\n");
    fflush(stdout);
}

int handle_args(int argc, char *argv[]) {
    if (argc > 2) {
        printf("Too many arguments!\n");
        fflush(stdout);
    } else if (argc > 1) {
        if (strcmp(argv[1], "-D") == 0 || strcmp(argv[1], "--daemon") == 0) {
            return daemonize();
        } else if (strcmp(argv[1], "-C") == 0 || strcmp(argv[1], "--client") == 0) {
            return write_to_socket();
        } else {
            printf("Invalid arguments\n");
            fflush(stdout);
        }
    }
    return 1;
}

/*
 * handler for SIGTERM signal. will delete(?) the unix domain socket file and kill with SIGKILL the daemon server
 */
void stop_server(int sig) {
    UNUSED(sig);

    printf("Stopping server...\n");
    fflush(stdout);

    // unlink(PID_PATH); // Get rid of pesky pidfile, socket
    // unlink(SOCK_PATH);
    // kill(0, SIGKILL); // Infanticide :(
    // exit(0);
    daemon_run = false;
}

int write_to_socket() {
    int fd;
    struct sockaddr_un addr;
    char buff[1024];
    int len;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCK_PATH);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        return 1;
    }

    printf("sending 'iccExchangeAPDU'\n");
    fflush(stdout);

    strcpy(buff, "iccExchangeAPDU");
    if (send(fd, buff, strlen(buff) + 1, 0) == -1) {
        perror("send");
        return 1;
    }

    printf("waiting for response:\n");
    fflush(stdout);

    if ((len = recv(fd, buff, 1024, 0)) < 0) {
        perror("recv");
        return 1;
    }

    printf("receive %d: %s\n", len, buff);
    fflush(stdout);

    /*
    while (len = recv(fd, &buff, 1024, 0), len > 0) {
        printf("Received len %d: %s\n", len, buff);
        // send(client_sockfd, &buff, len, 0);
    }
    */

    if (fd) {
        close(fd);
    }

    printf("Done\n");
    fflush(stdout);

    return 0;
}
