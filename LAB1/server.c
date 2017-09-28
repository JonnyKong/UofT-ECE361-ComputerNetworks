#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Server only takes 1 argument\n");
        exit(1);
    }
    char *port = argv[1];

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;  // will point to the results
    memset(&hints, 0, sizeof hints); // make sure the struct is empty

    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    int socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if ((status = bind(socketfd, servinfo->ai_addr, servinfo->ai_addrlen)) != 0) {
        fprintf(stderr, "bind error: %s\n", strerror(status));
        exit(1);
    }
    
    const int BACKLOG = 5;
    if ((status = listen(socketfd, BACKLOG)) != 0) {
        fprintf(stderr, "listen error: %s\n", strerror(status));
        exit(1);
    }

    struct sockaddr_storage host_addr;
    socklen_t addr_size = sizeof host_addr;
    int acceptfd = accept(socketfd, (struct sockaddr *)&host_addr, &addr_size);

    const int MAXLEN = 100;
    char buffer[MAXLEN];
    for (;;) {
        memset(buffer, 0, MAXLEN * sizeof(buffer)); // make sure the buffer is empty
        status = recv(acceptfd, buffer, MAXLEN, 0);
        if (status == 0) {
            break;
        } else if (status < 0) {
            fprintf(stderr, "listen error: %s\n", strerror(status));
            exit(1);
        } else {
            char* msg;
            int len;
            if (strcmp("ftp", buffer) == 0) {
                msg = "yes";
                len = strlen(msg);
            } else {
                msg = "no";
                len = strlen(msg);
            }
            status = send(acceptfd, msg, len, 0);
        }
    }

    close(socketfd);
    close(acceptfd);
    freeaddrinfo(servinfo); // free the linked-list
    return 0;
}

