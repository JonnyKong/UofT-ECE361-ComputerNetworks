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

	Packet packet;
	FILE *pFile = NULL;
	bool *fragRecv = NULL;
	for (;;) {
		if (recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen) == -1) {
			fprintf(stderr, "recvfrom error\n");
			exit(1);
		}
		stringToPacket(buf, &packet);
		if (!pFile) {
			pFile = fopen(packet.filename, "wb");
		}
		if (!fragRecv) {
			fragRecv = (bool *) malloc(packet.total_frag * sizeof(fragRecv));
			for (int i = 0; i < packet.total_frag; i++) {
				fragRecv[i] = false;
			}
		}
		if (!fragRecv[packet.frag_no]) {
			if (fwrite(packet.filedata, sizeof(char), packet.size, pFile) != packet.size) {
				fprintf(stderr, "fwrite error\n");
				exit(1);
			}
			fragRecv[packet.frag_no] = true;
		}
		strcpy(packet.filedata, "ACK");
		packetToString(&packet, buf);
		if ((sendto(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr))) == -1) {
			fprintf(stderr, "sendto error\n");
			exit(1);
		}
		if (packet.frag_no == packet.total_frag) {
			fprintf(stdout, "File transfer completed in %s\n", packet.filename);
		}
	}

	close(sockfd);
	fclose(pFile);
	free(fragRecv);
	return 0;
}