#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        fprintf(stdout, "usage: deliver -<server address> -<server port number>\n");
        exit(1);
    }

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;  // will point to the results
    
    memset(&hints, 0, sizeof hints); // make sure that the structs are empty
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    // get ready to connect
    if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo )) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos
    int sockfd; // socket file descriptor
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(status));
        exit(1);
    }

    const int BUF_SIZE = 100;
    char buf[BUF_SIZE] = {0};
    char filename[BUF_SIZE] = {0};

    // get user input
    fgets(buf, BUF_SIZE, stdin);

    int cursor = 0;
    while(buf[cursor] == ' ') { // skip whitespaces at start
        cursor++;
    }
    if(tolower(buf[cursor]) == 'f' && tolower(buf[cursor + 1]) == 't' && tolower(buf[cursor + 2]) == 'p') {
        cursor += 3;
        while (buf[cursor] == ' ') { // skip whitespaces inbetween
            cursor++;
        }
        char *token = strtok(buf + cursor, "\r\t\n ");
        strncpy(filename, token, BUF_SIZE);
    } else {
        fprintf(stderr, "Starter \"ftp\" undeteceted.\n");
        exit(1);
    }

    // Eligibility check of the file  
    if(access(filename, F_OK) == -1) {
        fprintf(stderr, "File \"%s\" doesn't exist.\n", filename);
        exit(1);
    }

    int numbytes;
    if((numbytes = sendto(sockfd, "ftp", strlen("ftp"), 0, servinfo->ai_addr, servinfo->ai_addrlen)) == -1){
        fprintf(stderr, "sendto error\n");
        exit(1);
    };

    memset(buf, 0, BUF_SIZE); // clean the buffer
    struct sockaddr_in serv_addr;
    socklen_t serv_addr_size = sizeof(serv_addr);
    if((numbytes = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &serv_addr, &serv_addr_size)) == -1) {
        fprintf(stderr, "recvfrom error\n");
        exit(1);
    }

    if(strcmp(buf, "yes") == 0) {
        fprintf(stdout, "A file transfer can start\n");
    }

    freeaddrinfo(servinfo);
    close(sockfd);
    
    return 0;
}