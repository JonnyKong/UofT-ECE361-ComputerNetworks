#include <stdio.h>
#include <stdlib.h>
#include "packet.h"
#include "user.h"

#define ULISTFILE "userlist.txt"

User *userList;     // All users and passwords

int main() {

    // Open and load userlist at server startup
    FILE *fp;
    if((fp = fopen(ULISTFILE, "r")) == NULL) {
        fprintf(stderr, "Can't open input file %s\n", ULISTFILE);
    }
    userList = init_userlist(fp);
    fclose(fp);

    // 

















    destroy_userlist(userList);
    return 0;
}