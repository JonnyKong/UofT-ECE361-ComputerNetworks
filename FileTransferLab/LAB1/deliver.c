#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stdout, "usage: deliver -<server address> -<server port number>\n");
        exit(0);
    }
    int port = atoi(argv[2]);
 
    int sockfd;
    // open socket (DGRAM)
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "socket error\n");
        exit(1);
    }
    
    struct sockaddr_in serv_addr;
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_aton(argv[1] , &serv_addr.sin_addr) == 0) {
        fprintf(stderr, "inet_aton error\n");
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
    // send the message
    if ((numbytes = sendto(sockfd, "ftp", strlen("ftp") , 0 , (struct sockaddr *) &serv_addr, sizeof(serv_addr))) == -1) {
        fprintf(stderr, "sendto error\n");
        exit(1);
    }

    memset(buf, 0, BUF_SIZE); // clean the buffer
    socklen_t serv_addr_size = sizeof(serv_addr);
    if((numbytes = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &serv_addr, &serv_addr_size)) == -1) {
        fprintf(stderr, "recvfrom error\n");
        exit(1);
    }

    if(strcmp(buf, "yes") == 0) {
        fprintf(stdout, "A file transfer can start\n");
    }

    close(sockfd);
    
    return 0;
}