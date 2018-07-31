#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include "packet.h"

int main(int argc, char const *argv[])
{
    if (argc != 2) { 
        printf("usage: server -<UDP listen port>\n");
        exit(0);
    }
    int port = atoi(argv[1]);

    int sockfd;	
    // open socket (DGRAM)
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        printf("socket error\n");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);	
    memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));

    // bind to socket
    if ((bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) == -1) {
        printf("bind error\n");
        exit(1);
    }

    char buf[BUF_SIZE] = {0};
    struct sockaddr_in cli_addr; 
    socklen_t clilen = sizeof(cli_addr); // length of client info
    // recvfrom the client and store info in cli_addr so as to send back later
    if (recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen) == -1) {
        printf("recvfrom error\n");
        exit(1);
    }

    // send message back to client based on message recevied
    if (strcmp(buf, "ftp") == 0) {
        if ((sendto(sockfd, "yes", strlen("yes"), 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr))) == -1) {
            printf("sendto error\n");
            exit(1);
        }
    } else {
        if ((sendto(sockfd, "no", strlen("no"), 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr))) == -1) {
            printf("sendto error\n");
            exit(1);
        }
    }
    Packet packet;
    packet.filename = (char *) malloc(BUF_SIZE);
    char filename[BUF_SIZE] = {0};
    FILE *pFile = NULL;
    bool *fragRecv = NULL;
    for (;;) {
        if (recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen) == -1) {
            printf("recvfrom error\n");
            exit(1);
        }		
        stringToPacket(buf, &packet);
        if (!pFile) {
            strcpy(filename, packet.filename);	
            while (access(filename, F_OK) == 0) {
                char *pSuffix = strrchr(filename, '.');
                if (pSuffix != NULL) {
                    char suffix[BUF_SIZE + 1] = {0};
                    strncpy(suffix, pSuffix, BUF_SIZE - 1);
                    *pSuffix = '\0';
                    strcat(filename, " copy");
                    strcat(filename, suffix);
                } else {
                    strcat(filename, " copy");	
                }
            } 
            pFile = fopen(filename, "w");
        }
        if (!fragRecv) {
            fragRecv = (bool *) malloc(packet.total_frag * sizeof(fragRecv));
            for (int i = 0; i < packet.total_frag; i++) {
                fragRecv[i] = false;
            }
        }
        if (!fragRecv[packet.frag_no - 1]) {	
            int numbyte = fwrite(packet.filedata, sizeof(char), packet.size, pFile);
            if (numbyte != packet.size) {
                printf("fwrite error\n");
                exit(1);
            } 
            fragRecv[packet.frag_no - 1] = true;
        }
        strcpy(packet.filedata, "ACK");

        packetToString(&packet, buf);
        if ((sendto(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr))) == -1) {
            printf("sendto error\n");
            exit(1);
        }

        if (packet.frag_no == packet.total_frag) {
            bool finished = true;
            for (int p = 0; p < packet.total_frag; p++) {
                finished &= fragRecv[p];			
            }
            if (finished) break;
        }
    }

    // wait for FIN message
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 999999;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "setsockopt failed\n");
    }

    for (;;) {
        if (recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen) == -1) {
            printf("timeout or recv error when waiting for FIN message\nForce quit\n");
            break;
        }		
        stringToPacket(buf, &packet);	
        if (strcmp(packet.filedata, "FIN") == 0) { 
            printf("File %s transfer completed\n", filename);
            break;
        } else {
            strcpy(packet.filedata, "ACK");
            packetToString(&packet, buf);
            if ((sendto(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr))) == -1) {
                printf("sendto error\n");
                exit(1);
            }
        } 
    }

    close(sockfd);
    fclose(pFile);
    free(fragRecv);
    free(packet.filename);
    return 0;
}
