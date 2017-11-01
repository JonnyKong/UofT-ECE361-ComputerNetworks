#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNAMELEN 32
#define PWDLEN 32
#define IPLEN 16

typedef struct _User {
    // Username and passwords
    char uname[UNAMELEN];
    char pwd[PWDLEN];

    // Client status (if valid)
    int session_id;
    char IP[IPLEN];
    int port;

    // Linked list support
    struct _User *next;     
} User;


// Read user database from file
User *init_userlist(FILE *fp) {
    int r;
    User *root = NULL;

    while(1) {
        User *usr = calloc(1, sizeof(User));
        r = fscanf(fp, "%s %s\n", usr -> uname, usr -> pwd);
        if(r == EOF) {
            free(usr);
            break;
        }
        usr -> next = root;
        root = usr;        
    }
    return root;
}


// Free a linked list of users
void destroy_userlist(User *root) {
    User *current = root;
    User *next = current;
    while(current != NULL) {
        next = current -> next;
        free(current);
        current = next;
    }
    // printf("List Destroyed\n");
}