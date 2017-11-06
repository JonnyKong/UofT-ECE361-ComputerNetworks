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

// Remove user from a list based on username
User *remove_user(User *userList, User const *usr) {
    if(userList == NULL) return NULL;

    // First in list
    else if(strcmp(userList -> uname, usr -> uname) == 0) {
        User *cur = userList -> next;
        free(userList);
        return cur;
    }

    else {
        User *cur = userList;
        User *prev;
        while(cur != NULL) {
            if(strcmp(cur -> uname, usr -> uname) == 0) {
                prev -> next = cur -> next;
                free(cur);
                break;
            } else {
                prev = cur;
                cur = cur -> next;
            } 
        }
        return userList;
    }
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
}

// Check username and password of user (valid, connected, logged in, etc)
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

// Check if user is in list (matches username)
bool in_list(const User *userList, const User *usr) {
    const User *current = userList;
    while(current != NULL) {
        if(strcmp(current -> uname, usr -> uname) == 0) {
            return 1;    
        }
        current = current -> next;
    }
    return 0;
}