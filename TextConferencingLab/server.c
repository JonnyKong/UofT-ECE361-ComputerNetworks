#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "packet.h"
#include "user.h"

#define ULISTFILE "userlist.txt"
#define INBUF_SIZE 64

User *userList;             // List of all users and passwords
User *userConnected;        // List of users connected (up to date)
int servport;               // Server port

char inBuf[INBUF_SIZE] = {0};  // Input buffer

int main() {
    // Get user input
    memset(inBuf, 0, INBUF_SIZE * sizeof(char));
    fgets(inBuf, INBUF_SIZE, stdin);
    char *pch;
    if((pch = strstr(inBuf, "server ")) != NULL) {
        pch += 7;
        servport = atoi(pch);
    } else {
        fprintf(stderr, "Starter \"server\" undetected.\n");
        exit(1);
    }
    // printf("Server starting at port %d\n", servport);


    // Open and load userlist at server startup
    FILE *fp;
    if((fp = fopen(ULISTFILE, "r")) == NULL) {
        fprintf(stderr, "Can't open input file %s\n", ULISTFILE);
    }
    userList = init_userlist(fp);
    fclose(fp);















    destroy_userlist(userList);
    return 0;
}