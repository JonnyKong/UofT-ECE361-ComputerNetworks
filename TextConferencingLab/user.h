#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define UNAMELEN 32
#define PWDLEN 32
#define IPLEN 16

typedef struct _User {
    // Username and passwords
    char uname[UNAMELEN];   // Valid if logged in
    char pwd[PWDLEN];       // Valid if logged in

    // Client status
    int session_id;     // Valid if joined a session
    // char IP[IPLEN];     // Valid if connected
    // int port;           // Valid if connected
    int sockfd;         // Valid if connected
    pthread_t p;        // Valid if connected

    // Linked list support
    struct _User *next;     

} User;

// Add user to a list
User *add_user(User* userList, User *newUsr) {
    newUsr -> next = userList;
    return newUsr;
}


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
        // usr -> next = root;
        // root = usr;        
        root = add_user(root, usr);
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

// Check if usr is in list (valid, connected, logged in, etc)
bool is_valid_user(const User *userList, const User *usr) {
    const User *current = userList;
    while(current != NULL) {
        if(strcmp(current -> uname, usr -> uname) == 0 && strcmp(current -> pwd, usr -> pwd) == 0) {
            return 1;    
        }
        current = current -> next;
    }
    return 0;
}