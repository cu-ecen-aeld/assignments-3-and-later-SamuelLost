#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>

#define SOCKET_FAIL -1
#define RET_OK 0
#define RECV_BUFF_SIZE 1024
#define READ_BUFF_SIZE 1024
#define BACKLOG 10

char *pc_data_file = "/var/tmp/aesdsocketdata";
FILE *pf_data_file = NULL;

char *pc_port = "9000";
struct addrinfo *servinfo = NULL;
int sfd = 0;
int sockfd = 0;


/* completing any open connection operations,
 * closing any open sockets, and deleting the file /var/tmp/aesdsocketdata*/
void exit_cleanup(void) {
    /* Cleanup with reentrant functions only*/
    /* Remove datafile */
    if (pf_data_file != NULL) {
        close(fileno(pf_data_file));
        unlink(pc_data_file);
    }

    /* Close socket */
    if (sfd >= 0) {
        close(sfd);
    }

    /* Close socket */
    if (sockfd >= 0) {
        close(sockfd);
    }
}

/* Signal actions with cleanup */
void signal_handler(int signal_number, siginfo_t *info, void *context)
{
    int errno_bkp = errno;

    if (signal_number == SIGINT) {
        syslog(LOG_DEBUG, "Caught signal, exiting");
        printf("caught SIGINT\n");
    } else if (signal_number == SIGTERM) {
        syslog(LOG_DEBUG, "Caught signal, exiting");
        printf("caught SIGTERM\n");
    }
    errno = errno_bkp;
    exit_cleanup();
    exit(EXIT_SUCCESS);
}

void do_exit(int exitval) {
    exit_cleanup();
    exit(exitval);
}

/* Description:
 * Setup signals to catch
 *
 * Return:
 * - errno on error
 * - RET_OK when succeeded
 */
int setup_signals(void) {
    /* SIGINT or SIGTERM terminates the program with cleanup */
    struct sigaction sa = {.sa_sigaction = &signal_handler};

    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("Setting up SIGINT");
        return errno;
    }

    if (sigaction(SIGTERM, &sa, NULL) != 0) {
        perror("Setting up SIGTERM");
        return errno;
    }
    return RET_OK;
}

/* Description:
 * Setup datafile to use
 *
 * Return:
 * - errno on error
 * - RET_OK when succeeded
 */
int setup_datafile(void) {
    /* Create and open destination file */
    if ((pf_data_file = fopen(pc_data_file, "w+")) == NULL) {
        perror("fopen: %s");
        printf("Error opening: %s", pc_data_file);
        return errno;
    }

    return RET_OK;
}

/* Description:
 * Setup socket handling
 * https://beej.us/guide/bgnet/html/split/system-calls-or-bust.html#system-calls-or-bust
 *
 * Return:
 * - errno on error
 * - RET_OK when succeeded
 */
int setup_socket(void) {

    struct addrinfo hints;
    int yes = 1; // for setsockopt() SO_REUSEADDR, below

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // bind to all interfaces

    if ((getaddrinfo(NULL, pc_port, &hints, &servinfo)) != 0) {
        perror("getaddrinfo");
        return errno;
    }

    if ((sfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
        perror("socket");
        return errno;
    }

    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        perror("setsockopt");
        return errno;
    }

    if (bind(sfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        perror("bind");
        return errno;
    }

    /* Not needed anymore */
    freeaddrinfo(servinfo);

    if (listen(sfd, BACKLOG) < 0) {
        perror("listen");
        return errno;
    }

    return RET_OK;
}

/* Description:
 * Send complete file through socket to the client
 *
 * Return:
 * - errno on error
 * - RET_OK when succeeded
 */
int file_send(void) {
    /* Send complete file */
    fseek(pf_data_file, 0, SEEK_SET);
    char read_buffer[READ_BUFF_SIZE];
    while (!feof(pf_data_file))
    {
        // NOTE: fread will return nmemb elements
        // NOTE: fread does not distinguish between end-of-file and error,
        int rc_read = fread(read_buffer, 1, sizeof(read_buffer), pf_data_file);
        if (ferror(pf_data_file) != 0) {
            perror("read");
            return errno;
        }

        if (send(sockfd, read_buffer, rc_read, 0) < 0) {
            perror("send");
            return errno;
        }
    }

    return RET_OK;
}

/* Description:
 * Write buff with size to datafile
 *
 * Return:
 * - errno on error
 * - RET_OK when succeeded
 */
int file_write(void *buff, int size) {
    /* Append received data */
    fseek(pf_data_file, 0, SEEK_END);
    fwrite(buff, size, 1, pf_data_file);
    if (ferror(pf_data_file) != 0) {
        perror("write");
        return errno;
    }

    return RET_OK;
}

int daemonize(void) {
    
    umask(0); 

    pid_t pid;
    if ((pid = fork()) < 0) {
        perror("fork");
        return errno;
    } else if (pid != 0) {
        /* Exit parent */
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("setsid"); //perror("chdir")
        return errno;
    }

    if (chdir("/") < 0) {
        perror("chdir");
        return errno;
    }

    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    return RET_OK;
}

int main(int argc, char **argv)
{

    int deamon = false;
    int ret = 0;
    /* init syslog */
    openlog(NULL, 0, LOG_USER);

    if ((argc > 1) && strcmp(argv[0], "-d")) {
        deamon = true;
    }

    if (setup_signals() != 0) {
        do_exit(ret);
    }

    if (setup_datafile() != 0) {
        do_exit(ret);
    }

    /* Opens a stream socket, failing and returning -1 if any of the socket connection steps fail. */
    if (setup_socket() != 0) {
        do_exit(SOCKET_FAIL);
    }

    if (deamon) {
        printf("Demonizing, listening on port %s\n", pc_port);
        if (daemonize() != 0) {
            do_exit(ret);
        }
    }
    else {
        printf("Waiting for connections...\n");
    }

    /* Keep receiving clients */
    while (1) {

        /* Accept clients */
        struct sockaddr_storage their_addr;
        socklen_t addr_size;
        if ((sockfd = accept(sfd, (struct sockaddr *)&their_addr, &addr_size)) < 0) {
            perror("accept");
            sleep(1);
            continue;
        }

        /* Get IP connecting client */
        struct sockaddr_in *sin = (struct sockaddr_in *)&their_addr;
        unsigned char *ip = (unsigned char *)&sin->sin_addr.s_addr;
        syslog(LOG_DEBUG, "Accepted connection from %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

        /* Keep receiving data until error or disconnect*/
        int received = 0;
        char recv_buffer[RECV_BUFF_SIZE];
        while (1) {
            received = recv(sockfd, &recv_buffer, sizeof(recv_buffer), 0);
            if (received < 0) {
                perror("recv");
                do_exit(errno);
            } else if (received == 0) {
                close(sockfd);
                syslog(LOG_DEBUG, "Closed connection from %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
                break;
            } else if (received > 0) {

                char *new_line = NULL;
                new_line = strstr(recv_buffer, "\n");
                if (new_line == NULL) {
                    /* not end of message, write all */
                    int ret = 0;
                    if ((ret = file_write(recv_buffer, received)) != 0) {
                        do_exit(ret);
                    }
                } else {
                    /* end of message detected, write until message end */
                    int ret = 0;

                    // NOTE: Ee know that message end is in the buffer, so +1 here is allowed to
                    // also get the end of message '\n' in the file.
                    if ((ret = file_write(recv_buffer, (int)(new_line - recv_buffer + 1))) != 0) {
                        do_exit(ret);
                    }

                    if ((ret = file_send()) != 0) {
                        do_exit(ret);
                    }
                }
            }
        }
    }

    do_exit(RET_OK);
}