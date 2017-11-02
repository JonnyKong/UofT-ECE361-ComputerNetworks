#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <regex.h>

#define BUF_SIZE 1100
#define DATA_SIZE 1000

typedef struct tagPacket {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char *filename;
	char filedata[DATA_SIZE]; 
} Packet;

void packetToString(const Packet *packet, char *result) {
    
    // Initialize string buffer
    memset(result, 0, BUF_SIZE);

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

    memcpy(result + cursor, packet -> filedata, sizeof(char) * DATA_SIZE);

    // printf("%s\n", packet -> filedata);
    // printf("Length of data: %d\n", strlen(packet -> filedata));

}

void stringToPacket(const char* str, Packet *packet) {

    // Compile Regex to match ":"
    regex_t regex;
    if(regcomp(&regex, "[:]", REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
    }

    // Match regex to find ":" 
    regmatch_t pmatch[1];
    int cursor = 0;
    char buf[BUF_SIZE];

    // Match total_frag
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }
    memset(buf, 0, BUF_SIZE * sizeof(char));
    memcpy(buf, str + cursor, pmatch[0].rm_so);
    packet -> total_frag = atoi(buf);
    cursor += (pmatch[0].rm_so + 1);

    // Match frag_no
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }
    memset(buf, 0,  BUF_SIZE * sizeof(char));
    memcpy(buf, str + cursor, pmatch[0].rm_so);
    packet -> frag_no = atoi(buf);
    cursor += (pmatch[0].rm_so + 1);

    // Match size
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }
    memset(buf, 0, BUF_SIZE * sizeof(char));
    memcpy(buf, str + cursor, pmatch[0].rm_so);
    packet -> size = atoi(buf);
    cursor += (pmatch[0].rm_so + 1);

    // Match filename
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }


    memcpy(packet -> filename, str + cursor, pmatch[0].rm_so);
    packet -> filename[pmatch[0].rm_so] = 0;
    cursor += (pmatch[0].rm_so + 1);
    
    // Match filedata
    memcpy(packet -> filedata, str + cursor, packet -> size);

    // printf("total_frag:\t%d\n", packet -> total_frag);
    // printf("frag_no:\t%d\n", packet -> frag_no);
    // printf("size:\t%d\n", packet -> size);
    // printf("filename:\t%s\n", packet -> filename);
    // printf("filedata:\t%s\n", packet -> filedata);

}

void printPacket(Packet *packet) {
    printf("total_frag = %d,\n frag_no = %d, size = %d, filename = %s\n", packet->total_frag, packet->frag_no, packet->size, packet->filename);
    char data[DATA_SIZE + 1] = {0};
    memcpy(data, packet->filedata, DATA_SIZE);
    printf("%s", data);
}
