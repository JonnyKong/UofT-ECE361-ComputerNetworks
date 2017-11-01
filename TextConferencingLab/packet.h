#include <stdio.h>

#define MAX_NAME 32
#define MAX_DATA 32

typedef struct packet {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
} Packet;