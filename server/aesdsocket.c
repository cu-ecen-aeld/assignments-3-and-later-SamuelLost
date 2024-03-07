//Reference: https://stackoverflow.com/questions/54718687/how-do-i-use-sigaction-struct-sigaction-is-not-defined
#define _POSIX_C_SOURCE 200112L
#define _POSIX_SOURCE
//--------------------
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <bits/socket.h>

#define PORT 9000
#define MAX_CONNECTIONS 10
#define BUFFER_SIZE 512
const char* log_path = "/var/tmp/aesdsocketdata";
bool accept_connections = true;
extern int errno;
int signal_caught = 0;
int socket_fd = 0;

void signal_handler(int signal) {
    syslog(LOG_USER, "Caught signal: %d", signal);
    if(close(socket_fd) == -1) {
        syslog(LOG_USER, "Error closing socket: %s", strerror(errno));
    }
    closelog();
    remove(log_path);
    exit(EXIT_SUCCESS);
}

// reference : https://beej.us/guide/bgnet/html/
void *get_in_addr(struct sockaddr *sa) {
    if(sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]) {
    struct sockaddr_storage client_addr;
    const char *tag = "AESD-SOCKET";
    openlog(tag, LOG_PID, LOG_USER);
    struct addrinfo hints, *res;
    int status;
    
    struct sigaction sa = {.sa_handler = signal_handler};
    if(sigaction(SIGINT, &sa, NULL) == -1) {
        syslog(LOG_USER, "Error setting up signal handler: %s", strerror(errno));
        closelog();
        return -1;
    }
    if(sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_USER, "Error setting up signal handler: %s", strerror(errno));
        closelog();
        return -1;
    }

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    if((status = getaddrinfo(NULL, "9000", &hints, &res)) != 0) {
        syslog(LOG_USER, "Error getting address info: %s", gai_strerror(status));
        closelog();
        freeaddrinfo(res);
        return -1;
    }

    if((socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        syslog(LOG_USER, "Error creating socket: %s", strerror(errno));
        closelog();
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);

    if(argc > 1) {
        printf("Argument: %s\n", argv[1]);
        if(!strcmp(argv[1], "-d")) {
            syslog(LOG_USER, "Daemonizing the process");
            pid_t pid = fork();
            if(pid == -1) {
                syslog(LOG_USER, "Error forking the process: %s", strerror(errno));
                close(socket_fd);
                closelog();
                return -1;
            } else if (pid > 0) {
                printf("Parent process exiting\n");
                exit(EXIT_SUCCESS);
            } else {
                if(setsid() == -1) {
                    syslog(LOG_USER, "Error creating new session: %s", strerror(errno));
                    close(socket_fd);
                    closelog();
                    return -1;
                }
                chdir("/");
                // Redirecting standard input, output and error to /dev/null
                open("/dev/null", O_RDWR);
                dup(0); // stdin
                dup(0); // stdout
                dup(0); // stderr
            }
        }
    }
    
    if((status = listen(socket_fd, MAX_CONNECTIONS)) == -1) {
        syslog(LOG_USER, "Error listening on socket: %s", strerror(errno));
        close(socket_fd);
        closelog();
        return -1;
    }
    socklen_t client_addr_len = sizeof(client_addr);
    while(true) {
        int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if(client_fd == -1) {
            syslog(LOG_USER, "Error accepting connection: %s", strerror(errno));
            close(socket_fd);
            closelog();
            return -1;
        }
        char client_ip[INET6_ADDRSTRLEN];
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr*)&client_addr), client_ip, sizeof(client_ip));
        syslog(LOG_USER, "Accepted connection from %s", client_ip);
        char buffer[BUFFER_SIZE];
        bool packet_complete = false;
        int bytes_received, bytes_sent = 0;
        int log_fd = open(log_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if(log_fd == -1) {
            syslog(LOG_USER, "Error opening log file: %s", strerror(errno));
            close(client_fd);
            close(socket_fd);
            closelog();
            return -1;
        }

        while(!packet_complete) {
            bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if(bytes_received == -1) {
                syslog(LOG_USER, "Error receiving data: %s", strerror(errno));
                close(client_fd);
                break;
            } else if(bytes_received == 0 || (strchr(buffer, '\n') != NULL)) {
                packet_complete = true;
            }

            bytes_sent = write(log_fd, buffer, bytes_received);
            if(bytes_sent != bytes_received) {
                syslog(LOG_USER, "Error writing to log file: %s", strerror(errno));
                close(client_fd);
                close(log_fd);
                break;
            }
        }
        packet_complete = false;
        lseek(log_fd, 0, SEEK_SET);
        while(!packet_complete) {
            bytes_received = read(log_fd, buffer, BUFFER_SIZE);
            if(bytes_received == -1) {
                syslog(LOG_USER, "Error reading from log file: %s", strerror(errno));
                close(client_fd);
                close(log_fd);
                break;
            } else if(bytes_received == 0) {
                packet_complete = true;
            }
            bytes_sent = send(client_fd, buffer, bytes_received, 0);
            if(bytes_sent != bytes_received) {
                syslog(LOG_USER, "Error sending data: %s", strerror(errno));
                close(client_fd);
                close(log_fd);
                break;
            }
        }
        close(client_fd);
        close(log_fd);   
    }
    close(socket_fd);
    closelog();
    remove(log_path);
    return 0;    
}

