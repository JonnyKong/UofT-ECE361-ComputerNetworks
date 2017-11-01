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

#include "packet.h"

#define ALIVE 32


// Convert packet to string

void send_file(char *filename, int sockfd, struct sockaddr_in serv_addr) {

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
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "setsockopt failed\n");
    }
    int timesent = 0;   // Number of times a packet is sent
    socklen_t serv_addr_size = sizeof(serv_addr);

    Packet ack_packet;  // Packet receiveds
    ack_packet.filename = (char *)malloc(BUF_SIZE * sizeof(char));

    for(int packet_num = 1; packet_num <= total_frag; ++packet_num) {

        int numbytes;       // Return value of sendto and recvfrom
        ++timesent;

        // Send packets
        // printf("Sending packet #%d / %d...\n", packet_num, total_frag);
        if((numbytes = sendto(sockfd, packets[packet_num - 1], BUF_SIZE, 0 , (struct sockaddr *) &serv_addr, sizeof(serv_addr))) == -1) {
            fprintf(stderr, "sendto error for packet #%d\n", packet_num);
            exit(1);
        }

        // Receive acknowledgements
        memset(rec_buf, 0, sizeof(char) * BUF_SIZE);
        if((numbytes = recvfrom(sockfd, rec_buf, BUF_SIZE, 0, (struct sockaddr *) &serv_addr, &serv_addr_size)) == -1) {
            // Resend if timeout
            fprintf(stderr, "Timeout or recvfrom error for ACK packet #%d, resending attempt #%d...\n", packet_num--, timesent);
            if(timesent < ALIVE){
                continue;
            }
            else {
                fprintf(stderr, "Too many resends. File transfer terminated.\n");
                exit(1);
            }
        }
        
        stringToPacket(rec_buf, &ack_packet);
        
        // Check contents of ACK packets
        if(strcmp(ack_packet.filename, filename) == 0) {
            if(ack_packet.frag_no == packet_num) {
                if(strcmp(ack_packet.filedata, "ACK") == 0) {
                    // printf("ACK packet #%d received\n", packet_num);
                    timesent = 0;
                    continue;
                }
            }
        }

        // Resend packet
        fprintf(stderr, "ACK packet #%d not received, resending attempt #%d...\n", packet_num, timesent);
        --packet_num;

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
    end = clock();
    fprintf(stdout, "RTT = %f sec.\n", ((double) (end - start) / CLOCKS_PER_SEC));  

    if(strcmp(buf, "yes") == 0) {
        fprintf(stdout, "A file transfer can start\n");
    }
    else {
        fprintf(stderr, "Error: File transfer can't start\n");
    }


    // Begin sending file and check for acknowledgements
    send_file(filename, sockfd, serv_addr);
    
    // Sending Completed
    close(sockfd);
    printf("File Transfer Completed, SocketClosed.\n");
    
    return 0;
}
