#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define MAX_NAME 32
#define MAX_DATA 512

#define BUF_SIZE 600

enum msgType {
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_ACK,
    JN_NAK,
    LEAVE_SESS,
    NEW_SESS,
    NS_ACK,
    MESSAGE,
    QUERY,
    QU_ACK
};

typedef struct lab3message {
    unsigned int type;  // msgType
    unsigned int size;  // Size of data
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
} Packet;


/* Convert packet to fixed size string, with attributes separated
 * by colons
 */
void packetToString(const Packet *packet, char *dest) {
    
    //Initialize string buffer
    memset(dest, 0, sizeof(char) * BUF_SIZE);

    // Load data into string
    int cursor = 0;
    sprintf(dest, "%d", packet -> type);
    cursor = strlen(dest);
    memcpy(dest + cursor++, ":", sizeof(char));

    sprintf(dest + cursor, "%d", packet -> size);
    cursor = strlen(dest);
    memcpy(dest + cursor++, ":", sizeof(char));

    // Copy only valid part of strings so as to support regex
    sprintf(dest + cursor, "%s", packet -> source);
    cursor = strlen(dest);
    memcpy(dest + cursor++, ":", sizeof(char));

    memcpy(dest + cursor, packet -> data, strlen((char *)(packet -> data)));
    cursor = strlen(dest);
}


// Convert str to packet
void stringToPacket(const char *str, Packet *dest_packet) {

    // Compile Regex to match ":"
    regex_t regex;
    if(regcomp(&regex, "[:]", REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
    }

    // Match regex to find ":" 
    regmatch_t pmatch[1];
    int cursor = 0;
    const int regBfSz = MAX_DATA;
    char buf[regBfSz];     // Temporary buffer for regex matching        

    // Match type
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }
    memset(buf, 0, regBfSz * sizeof(char));
    memcpy(buf, str + cursor, pmatch[0].rm_so);
    dest_packet -> type = atoi(buf);
    cursor += (pmatch[0].rm_so + 1);

    // Match size
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }
    memset(buf, 0, regBfSz * sizeof(char));
    memcpy(buf, str + cursor, pmatch[0].rm_so);
    dest_packet -> size = atoi(buf);
    cursor += (pmatch[0].rm_so + 1);

    // Match source
    if(regexec(&regex, str + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }
    memcpy(dest_packet -> source, str + cursor, pmatch[0].rm_so);
    dest_packet -> source[pmatch[0].rm_so] = 0;
    cursor += (pmatch[0].rm_so + 1);

    // Match data
    memcpy(dest_packet -> data, str + cursor, dest_packet -> size);
}