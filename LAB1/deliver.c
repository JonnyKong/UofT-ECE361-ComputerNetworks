#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

#define BUFFER_SIZE 100

int main(int argc, char *argv[])
{
	struct addrinfo hints, *res, *p;
    int status;
    char send_msg[BUFFER_SIZE] = "ftp";     // Message to send
    char rec_msg[BUFFER_SIZE];              // Message received
    char file_name[BUFFER_SIZE];            // File name from user's input
    char input_check[BUFFER_SIZE] = "ftp";  // Check if user input begins with ftp
    int s;

	if (argc != 3) {
	    fprintf(stderr, "usage: showip hostname\n");
	    return 1;
    }

    // Check input begins with ftp
    scanf("%s", file_name);
    if(strcmp(file_name, "ftp") != 0) {
        fprintf(stderr, "usage: wrong input\n");
        exit(1);
    }
    scanf("%s", file_name);
    
    // If file does not exist
    if(access(file_name, F_OK) == -1) {
        fprintf(stderr, "File does not exist!\n");
        exit(2);
    }
    
    // Setup destination address info
    memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_DGRAM;
    if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        puts("Error getting server address\n");
        exit(3);
    }

    // Setup socket
    if((s = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol)) < 0) {
        fprintf(stderr, "socket:");
        puts("Error setting up socket\n");
        exit(4);
    }

    // Establish connection
    if((status = connect(s, res->ai_addr, res->ai_addrlen)) != 0) {
        fprintf(stderr, "connect:");
        puts("Error establishing connection\n");
        exit(5);
    }

    // Send message
    if((status = send(s, send_msg, strlen(send_msg), 0)) < 0) {
        fprintf(stderr, "send:");
        puts("Error sending\n");
        exit(6);
    }

    // Receive message
    while(1){
        memset(rec_msg, 0, sizeof(char) * BUFFER_SIZE);
        status = recv(s, rec_msg, BUFFER_SIZE, 0);
        if(status == 0) {
            fprintf(stderr, "Error: Connection Closed\n");
            exit(0);
        }
        else if(status < 0) {
            fprintf(stderr, "Error: recv %d\n", status);
        }
    }

    if(strcmp(rec_msg, "yes") == 0) {
        puts("A file transfer can start\n");
    }
    else {
        exit(7);
    }

    // Close the socket
    close(s);
	return 0;
}
