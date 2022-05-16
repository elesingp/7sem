#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define localhost "127.0.0.1"

int signal_catch = 0;

void sigintfunc(int signal)
{
    signal_catch = 1;
}

void sigtermfunc(int signal)
{
    signal_catch = 1;
}

void close_socket(int socket_fd)
{
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
}

int main(int argc, char* argv[])
{

    struct sigaction sig_int = {.sa_handler = &sigintfunc};
    struct sigaction sig_term = {.sa_handler = &sigtermfunc};
    sigaction(SIGINT, &sig_int, NULL);
    sigaction(SIGTERM, &sig_term, NULL);

    int socket_fd = 0;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        exit(1);
    }

    struct sockaddr_in addrIpv4;
    addrIpv4.sin_family = AF_INET;
    addrIpv4.sin_port = htons(atoi(argv[1]));
    addrIpv4.sin_addr.s_addr = inet_addr(localhost);

    if (bind(socket_fd, (const struct sockaddr*)&addrIpv4, sizeof(addrIpv4)) ==
        -1) {
        close_socket(socket_fd);
        exit(1);
    }
    if (listen(socket_fd, SOMAXCONN) == -1) {
        close_socket(socket_fd);
        exit(1);
    }

    while (1) {
        if (signal_catch == 1) {
            close_socket(socket_fd);
            return 0;
        }
        int fd = accept(socket_fd, NULL, NULL);
        char buffer[PATH_MAX];
        char path[PATH_MAX];
        char path_other[PATH_MAX];
        int size_read = read(fd, buffer, sizeof(buffer));
        sscanf(buffer, "%*s %s", path);
        sprintf(path_other, "%s/%s", argv[2], path);
        struct stat file;
        stat(path_other, &file);
        if (access(path_other, F_OK) != 0) {
            write(
                    fd,
                    "HTTP/1.1 404 Not Found\r\n",
                    strlen("HTTP/1.1 404 Not Found\r\n"));
            write(
                    fd,
                    "Content-Length: 0\r\n\r\n",
                    strlen("Content-Length: 0\r\n\r\n"));
        } else if (access(path_other, R_OK) != 0) {
            write(
                    fd,
                    "HTTP/1.1 403 Forbidden\r\n",
                    strlen("HTTP/1.1 403 Forbidden\r\n"));
            write(
                    fd,
                    "Content-Length: 0\r\n\r\n",
                    strlen("Content-Length: 0\r\n\r\n"));
        } else {
            char str[PATH_MAX];
            char buf[PATH_MAX];
            write(fd, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"));
            sprintf(str, "Content-Length: %d\r\n\r\n", file.st_size);
            write(fd, str, strlen(str));
            int f = open(path_other, O_RDONLY);
            int size_f = read(f, buf, sizeof(buf));
            write(fd, buf, size_f);
            close(f);
        }
        close_socket(fd);
    }
    close(socket_fd);
    return 0;
}