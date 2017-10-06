#include<stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <regex.h>

#include "packet.h"

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

Packet stringToPacket(const char* str) {
    
    // Initialize Packet
    Packet packet;

    // Compile Regex to match ":"
    regex_t regex;
    if(regcomp(&regex, "[:]", REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
    }


    // Match regex to find ":" 
    regmatch_t pmatch[1];
    int cursor = 0;
    char tmp[50];

    // Match total_frag
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
    }
    memset(tmp, 0, 50 * sizeof(char));
    memcpy(tmp, str + cursor, pmatch[0].rm_so);
    packet.total_frag = atoi(tmp);
    cursor += (pmatch[0].rm_so + 1);

    // Match frag_no
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
    }
    memset(tmp, 0, 50 * sizeof(char));
    memcpy(tmp, str + cursor, pmatch[0].rm_so);
    packet.frag_no = atoi(tmp);
    cursor += (pmatch[0].rm_so + 1);

    // Match size
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
    }
    memset(tmp, 0, 50 * sizeof(char));
    memcpy(tmp, str + cursor, pmatch[0].rm_so);
    packet.size = atoi(tmp);
    cursor += (pmatch[0].rm_so + 1);

    // Match filename
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
    }
    memcpy(packet.filename, str + cursor, pmatch[0].rm_so);
    cursor += (pmatch[0].rm_so + 1);

    // Match filedata
    memcpy(packet.filedata, str + cursor, packet.size);

    // printf("total_frag:\t%d\n", packet.total_frag);
    // printf("frag_no:\t%d\n", packet.frag_no);
    // printf("size:\t%d\n", packet.size);
    // printf("filename:\t%s\n", packet.filename);
    // printf("filedata:\t%s\n", packet.filedata);

    return packet;
}