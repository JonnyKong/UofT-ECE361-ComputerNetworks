#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    if (argc != 2) { 
        fprintf(stdout, "usage: server -<UDP listen port>\n");
        exit(0);
    }
    int port = atoi(argv[1]);

    int sockfd;	
    // open socket (DGRAM)
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        fprintf(stderr, "socket error\n");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);	
    memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));

    // bind to socket
    if ((bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) == -1) {
        fprintf(stderr, "bind error\n");
        exit(1);
    }

    const int BUF_SIZE = 100;
    char buf[BUF_SIZE] = {0};
    struct sockaddr_in cli_addr; 
    socklen_t clilen; // length of client info
    // recvfrom the client and store info in cli_addr so as to send back later
    if (recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen) == -1) {
        fprintf(stderr, "recvfrom error\n");
        exit(1);
    }

    // send message back to client based on message recevied
    if (strcmp(buf, "ftp") == 0) {
        if ((sendto(sockfd, "yes", strlen("yes"), 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr))) == -1) {
            fprintf(stderr, "sendto error\n");
            exit(1);
        }
    } else {
        if ((sendto(sockfd, "no", strlen("no"), 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr))) == -1) {
            fprintf(stderr, "sendto error\n");
            exit(1);
        }
    }

    close(sockfd);
    return 0;
}
