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
#include <errno.h>
#include <stdbool.h>

#include "packet.h"

#define ALIVE 32


// Convert packet to string

void send_file(clock_t initRTT, char *filename, int sockfd, struct sockaddr serv_addr) {
    // Open file
    FILE *fp;
    if((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Can't open input file %s\n", filename);
        exit(1);
    }
    // printf("Successfully opened file %s\n", filename);


    // Calculate total number of packets required
    fseek(fp, 0, SEEK_END);
    int total_frag = ftell(fp) / 1000 + 1;
    printf("Number of packets: %d\n", total_frag);
    rewind(fp); 


    // Read file into packets, and save in packets array
    char rec_buf[BUF_SIZE];                                 // Buffer for receiving packets
    char **packets = malloc(sizeof(char*) * total_frag);    // Stores packets for retransmitting

    for(int packet_num = 1; packet_num <= total_frag; ++packet_num) {

        // printf("Preparing packet #%d / %d...\n", packet_num, total_frag);

        // Create Packet
        Packet packet;
        memset(packet.filedata, 0, sizeof(char) * (DATA_SIZE));
        fread((void*)packet.filedata, sizeof(char), DATA_SIZE, fp);

        // Update packet info
        packet.total_frag = total_frag;
        packet.frag_no = packet_num;
        packet.filename = filename;
        if(packet_num != total_frag) {
            // This packet is not the last packet
            packet.size = DATA_SIZE;
        }
        else {
            // This packet is the last packet
            fseek(fp, 0, SEEK_END);
            packet.size = (ftell(fp) - 1) % 1000 + 1;
        }

        // Save packet to packets array
        packets[packet_num - 1] = malloc(BUF_SIZE * sizeof(char));
        packetToString(&packet, packets[packet_num - 1]);

    }    


    // Send packets and receive acknowledgements
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 999999;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "setsockopt failed\n");
    }
    socklen_t serv_addr_size = sizeof(serv_addr);

    Packet ack_packet;  // Packet received
    ack_packet.filename = (char *)malloc(BUF_SIZE * sizeof(char));

    // Setup congestion control
    clock_t estimatedRTT = 2 * initRTT;
	clock_t devRTT = initRTT;
    clock_t sampleRTT, dev;
    clock_t start, end;
	
	int timesent = 0;   // Number of times a packet is sent
	int numbytes;
	int packet_num = 1;
	bool istimeout;
	while (packet_num <= total_frag) {
		istimeout = false;
		memset(rec_buf, 0, sizeof(char) * BUF_SIZE);
		start = clock();
		if((numbytes = sendto(sockfd, packets[packet_num - 1], BUF_SIZE, 0 , &serv_addr, sizeof(serv_addr))) == -1) {
            fprintf(stderr, "sendto error for packet #%d\n", packet_num);
            exit(1);
        }
		for (;;) {
			// Receive acknowledgements
			if((numbytes = recvfrom(sockfd, rec_buf, BUF_SIZE, 0, &serv_addr, &serv_addr_size)) == -1) {
				timesent++;
				// Resend if timeout
		        fprintf(stderr, "Timeout or recvfrom error for ACK packet #%d, resending attempt #%d...\n", packet_num, timesent);
		        if(timesent <= ALIVE){
					istimeout = true;
					break;
		        } else {
		            fprintf(stderr, "Too many resends. File transfer terminated.\n");
		            exit(1);
		        }
		    }
			stringToPacket(rec_buf, &ack_packet);
			// Check contents of ACK packets
		    if(ack_packet.frag_no != packet_num) {
				printf("Not current ACK. Drop and wait for the next ACK.\n");
		    	continue;
		    } else {
				break;			
			}
		}
		end = clock();
		// Update congestion control
	    sampleRTT = end - start;
	    estimatedRTT = 0.875 * ((double) estimatedRTT) + (sampleRTT >> 3);
	    dev = (estimatedRTT > sampleRTT) ? (estimatedRTT - sampleRTT) : (sampleRTT - estimatedRTT);
	    devRTT = 0.75 * ((double) devRTT) + (dev >> 2);
        timeout.tv_usec = 20 * estimatedRTT + (devRTT << 2);
        /*printf("sampleRTT = %d\testimatedRTT = %d\tdev = %d\tdevRTT = %d\ttv_usec = %d\n", 
                sampleRTT, estimatedRTT, dev, devRTT, timeout.tv_usec);*/
		if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
	    	fprintf(stderr, "setsockopt failed\n");
		}
		if (!istimeout) {
			packet_num++;
			timesent = 0;
		} else {
			fprintf(stderr, "Packet #%d timeout, timeout reset to:\t%d usec\n", packet_num, timeout.tv_usec);		
		}
	}
	// send FIN message
	Packet fin;
	fin.total_frag = total_frag;
	fin.frag_no = 0;
	fin.size = DATA_SIZE;
	fin.filename = filename;
	strcpy(fin.filedata, "FIN");
	packetToString(&fin, rec_buf);
	if((numbytes = sendto(sockfd, rec_buf, BUF_SIZE, 0, &serv_addr, sizeof(serv_addr))) == -1) {
        fprintf(stderr, "sendto error for FIN message\n");
        exit(1);
    }

    // Free memory
    for(int packet_num = 1; packet_num <= total_frag; ++packet_num) {
        free(packets[packet_num - 1]);
    }
    free(packets);

    free(ack_packet.filename);

}

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stdout, "usage: deliver -<server address> -<server port number>\n");
        exit(0);
    }
    int port = atoi(argv[2]);

	struct addrinfo hints;
	memset(&hints, 0 , sizeof(hints)); // make sure that the structs are empty
	hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_ADDRCONFIG;

	struct addrinfo *servinfo = NULL;  // will point to the results
	int status;
	// get ready to connect
    if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
 
    int sockfd;
    // open socket (DGRAM)
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        fprintf(stderr, "socket error\n");
        exit(1);
    }
	struct sockaddr serv_addr = *(servinfo->ai_addr);

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
    clock_t start, end;  // timer variables
    start = clock();
    if ((numbytes = sendto(sockfd, "ftp", strlen("ftp") , 0 , &serv_addr, sizeof(serv_addr))) == -1) {
        fprintf(stderr, "sendto error\n");
        exit(1);
    }
    
    memset(buf, 0, BUF_SIZE); // clean the buffer
    socklen_t serv_addr_size = sizeof(serv_addr);
    if((numbytes = recvfrom(sockfd, buf, BUF_SIZE, 0, &serv_addr, &serv_addr_size)) == -1) {
        fprintf(stderr, "recvfrom error\n");
        exit(1);
    }
    end = clock();

	clock_t initRTT = end - start;
    fprintf(stdout, "RTT = %lu usec\n", initRTT);  

    if(strcmp(buf, "yes") == 0) {
        fprintf(stdout, "A file transfer can start\n");
    }
    else {
        fprintf(stderr, "Error: File transfer can't start\n");
    }


    // Begin sending file and check for acknowledgements
    send_file(initRTT, filename, sockfd, serv_addr);

    // Sending Completed
    close(sockfd);
	freeaddrinfo(servinfo);
    printf("File Transfer Completed, Socket Closed.\n");
    
    return 0;
}
