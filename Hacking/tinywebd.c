#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "hacking.h"
#include "hacking-network.h"

#define PORT 80 // Port connecting to
#define WEBROOT "./webroot" // Root directory of webserver
#define LOGFILE "/var/log/tinywebd.log" // log filename

int logfd, sockfd;
void handle_connection(int, struct sockaddr_in *, int);
int get_file_size(int);
void timestamp(int);

// Function called when process is killed
void handle_shutdown(int signal) {
    timestamp(logfd);
    write(logfd, "Shutting Down.\n", 16);
    close(logfd);
    close(sockfd);
    exit(0);
}

int main(void) {
    int new_sockfd, yes=1;
    struct sockaddr_in host_addr, client_addr;
    socklen_t sin_size;

    logfd = open(LOGFILE, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    if(logfd == -1) {
        fatal("opening log file");
    }
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fatal("in socket");
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        fatal("setting socket option SO_REUSEADDR");
    }
    printf("Starting tiny web daemon.\n");
    if (daemon(1, 0) == -1) {
        fatal("forking to daemon process");
    }
    signal(SIGTERM, handle_shutdown);
    signal(SIGINT, handle_shutdown);

    timestamp(logfd);

    write(logfd, "Starting up.\n", 15);
    host_addr.sin_family = AF_INET;         // Host byte order
    host_addr.sin_port = htons(PORT);       // Short, network byte order
    host_addr.sin_addr.s_addr = INADDR_ANY; // Automatically fill with my IP
    memset(&(host_addr.sin_zero), '\0', 8); // Zero the rest of the struct

    if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1) {
        fatal("binding to socket");
    }

    if (listen(sockfd, 20) == -1) {
        fatal("listening on socket");
    }

    while(1) {
        sin_size = sizeof(struct sockaddr_in);
        new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if(new_sockfd == -1) {
            fatal("accepting connection");
        }
        handle_connection(new_sockfd, &client_addr, logfd);
    }
    return 0;
}

/* Function handles connection on socket from client address and logs it.
 * It is processed as a web request and this function replies over the
 * connected socket. The passed socket is then closed at the functions end.
 */
 void handle_connection(int sockfd, struct sockaddr_in *client_addr_ptr, int logfd) {
     unsigned char *ptr, request[500], resource[500], log_buffer[500];
     int fd, length;

     length = recv_line(sockfd, request);

     sprintf(log_buffer, "From %s:%d \"%s\"\t", inet_ntoa(client_addr_ptr->sin_addr),
ntohs(client_addr_ptr->sin_port), request);

    ptr = strstr(request, " HTTP/");
    if (ptr == NULL) {
        strcat(log_buffer, " NOT HTTP!\n");
    } else {
        *ptr = 0;
        ptr = NULL;
        if (strncmp(request "GET ", 4) == 0) {
            ptr = request + 4;
        }
        if (strncmp(request "HEAD ", 5) == 0) {
            ptr = request + 5;
        }
        if (ptr == NULL) {
            strcat(log_buffer, " UNKNOWN REQUEST!\n");
        } else {
            if (ptr[strlen(ptr) - 1] == '/') {
                strcat(ptr, "index.html");
            }
            strcpy(resource, WEBROOT);
            strcat(resource, ptr);
            fd = open(resource, O_RDONLY, 0)

            if (fd == -1) {
                strcat(log_buffer, " 404 Not Found\n");
                send_string(sockfd, "HTTP/1.0 404 NOT FOUND\r\n");
                send_string(sockfd, "Server: Tiny webserver\r\n\r\n");
                send_string(sockfd, "<html><head><title>404 Not Found</title></head>");
                send_string(sockfd, "<body><h1>URL not found</h1></body></html>\r\n");
            } else {
                strcat(log_buffer, " 200 OK\n");
                send_string(sockfd, "HTTP/1.0 200 OK\r\n");
                send_string(sockfd, "Server: Tiny webserver\r\n\r\n");
                if (ptr == request + 4) {
                    if ((length = get_file_size(fd)) == -1) {
                        fatal("getting resource file size");
                    }
                    if ((ptr = (unsigned char *) malloc(length)) == NULL) {
                        fatal("allocating memory for reading resource");
                    }
                    read(fd, ptr, length);
                    send(sockfd, ptr, length, 0);
                    free(ptr);
                }
                close(fd);
            }
        }
    }
    timestamp(logfd);
    length = strlen(log_buffer);
    write(logfd, log_buffer, length);

    shutdown(sockfd, SHUT_RDWR);
}

/* Function accepts file descriptor and returns
 * the size of the associated file. REturns -1 on failure.
 */
int get_file_size(int fd) {
    struct stat stat_struct;
    if (fstat(fd, &stat_struct) == -1) {
        return -1;
    }
    return (int) stat_struct.st_size;
}

/* Function writes a timestamp string to open file descriptor
 * that it is passed to.
 */
void timestamp(fd) {
    time_t now;
    struct tm *time_struct;
    int length;
    char time_buffer[40];

    time(&now);
    time_struct = localtime((const time_t *)&now);
    length = strftime(time_buffer, 40, "%m/%d/%Y %H:%M:%S> ", time_struct);
    write(fd, time_buffer, length);
}
