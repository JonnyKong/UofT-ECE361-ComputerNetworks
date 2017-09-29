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
		exit(1);
	}
    
	int sockfd;	// listen on socket sockfd
	int port = atoi(argv[1]);	// port to listen on
	struct sockaddr_in serv_addr, cli_addr; // structs to store server and client address info
	socklen_t clilen; // length of client info

	const int BUF_SIZE = 256;
	char buf[BUF_SIZE];	// buffer to store client's message

	bzero(buf, BUF_SIZE);
	// open socket (DGRAM)
	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("Can't open socket at server\n");
		exit(1);
	}
	
    fprintf(stderr, "2\n");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;	// don't need to bind socket to specific IP
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));

	// bind to socket
	if ((bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) == -1) {
		printf("Can't bind socket\n");
		exit(1);
	}
    fprintf(stderr, "3\n");
	clilen = sizeof(cli_addr);
    
	// receive from the client and store info in cli_addr for sending purposes
	if (recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen) == -1) {
		printf("Connection lost\n");
		exit(1);
	}
    fprintf(stderr, "4\n");
	// send message to client based on message recevied
	if (strcmp(buf, "ftp") == 0) {
		if ((sendto(sockfd, "yes", strlen("yes"), 0, (struct sockaddr *) &cli_addr, sizeof(serv_addr))) == -1) {
			printf("Can't send bytes from server\n");
			exit(1);
		}
        printf("Connection established\n");
	} else {
		if ((sendto(sockfd, "no", strlen("no"), 0, (struct sockaddr *) &cli_addr, sizeof(serv_addr))) == -1) {
			printf("Can't send bytes from server\n");
			exit(1);
		}
        printf("Connection not established\n");
	}
    fprintf(stderr, "5\n");
	// close Connection
	close(sockfd);
	return 0;
}