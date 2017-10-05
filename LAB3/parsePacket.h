#include<stdio.h>
#include <string.h> 
#include <stdlib.h>
#define SEND_BUF_SIZE 1000

char *packetToString(Packet *packet) {
    
        // Initialize string buffer
        const int buffer_size = SEND_BUF_SIZE + 100;
        char* result = malloc(buffer_size * sizeof(char));
        memset(result, 0, sizeof(char) * buffer_size);
    
        // Load data into string
        int cursor = 0;
        sprintf(result, "%d", packet -> total_frag);
        cursor = strlen(result);
        memcpy(result + cursor, ":", sizeof(char));
        ++cursor;
        
        sprintf(result + cursor, "%d", packet -> frag_no);
        cursor = strlen(result);
        memcpy(result + cursor, ":", sizeof(char));
        ++cursor;
    
        sprintf(result + cursor, "%d", packet -> size);
        cursor = strlen(result);
        memcpy(result + cursor, ":", sizeof(char));
        ++cursor;
    
        sprintf(result + cursor, "%s", packet -> filename);
        cursor += strlen(packet -> filename);
        memcpy(result + cursor, ":", sizeof(char));
        ++cursor;
    
        memcpy(result + cursor, packet -> filedata, sizeof(char) * SEND_BUF_SIZE);
    
        // printf("%s\n", packet -> filedata);
        // printf("Length of data: %d\n", strlen(packet -> filedata));
        return result;
    
    }