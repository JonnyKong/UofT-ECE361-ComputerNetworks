#ifndef USER_H_
#define USER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>

// #include "session.h"


#define UNAMELEN 32
#define PWDLEN 32
#define IPLEN 16

typedef struct _Session Session;
typedef struct _User {
    // Username and passwords
    char uname[UNAMELEN];   // Valid if logged in
    char pwd[PWDLEN];       // Valid if logged in

    // Client status
    int sockfd;         // Valid if connected
    pthread_t p;        // Valid if connected
    bool inSession;

    // Linked list support
    Session *sessJoined;    // Valid in userLoggedin
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




typedef struct _Session {
	int sessionId;

	//Linked list support
	struct _Session *next;
	struct _User *usr;	// List of users joined this session

} Session;

/* Check if a session is valid.
 * Return pointer to this session for in_Session()
 */
Session *isValidSession(Session *sessionList, int sessionId) {
	Session *current = sessionList;
	while(current != NULL) {
		if(current -> sessionId == sessionId) {
			return current;
		}
		current = current -> next;
	}
	return NULL;
}

bool inSession(Session *sessionList, int sessionId, const User *usr) {
	Session *session = isValidSession(sessionList, sessionId);
	if(session != NULL) {
		// Session exists, then check if user is in session
		return in_list(session -> usr, usr);
	} else {
		return 0;
	}
}

/* Initialize new session.
 * Session ends as last user leaves
 */
Session *init_session(Session *sessionList, const int sessionId) {
	Session *newSession = calloc(sizeof(Session), 1);
	newSession -> sessionId = sessionId;
	// Insert new session to head of the list
	newSession -> next  = sessionList;
	return newSession;
}


/* Insert new user into global session_list.
 * Have to insert usr into session list of each corresponding user
 * in its thread.
 */

Session *join_session(Session *sessionList, int sessionId, const User *usr) {
	// Check session exists outside & Find current session
	Session *cur = isValidSession(sessionList, sessionId);
	assert(cur != NULL);

	// Malloc new user
	User *newUsr = malloc(sizeof(User));
	memcpy((void *)newUsr, (void *)usr, sizeof(User));

	// Insert into session list
	cur -> usr = add_user(cur -> usr, newUsr);

	return sessionList;
}

/* Remove a session from list.
 * Called by new_client() and leave_session()
 */
Session* remove_session(Session* sessionList, int sessionId) {
	// Search for this session from sessionList
	assert(sessionList != NULL);
	
	// First in list
	if(sessionList -> sessionId == sessionId) {
		Session *cur = sessionList -> next;
		free(sessionList);
		return cur;
	}

	else {
		Session *cur = sessionList;
		Session *prev;
		while(cur != NULL) {
			if(cur -> sessionId == sessionId) {
				prev -> next = cur -> next;
				free(cur);
				break;
			} else {
				prev = cur;
				cur = cur -> next;
			}
		}
		return sessionList;
	}
}


Session *leave_session(Session *sessionList, int sessionId, const User *usr) {
	// Check session exists outside & Find current session
	Session *cur = isValidSession(sessionList, sessionId);
	assert(cur != NULL);

	// Remove user from this session
	assert(in_list(cur -> usr, usr));
	cur -> usr = remove_user(cur -> usr, usr);

	// If last user in session, the remove session
	if(cur -> usr == NULL) {
		sessionList = remove_session(sessionList, sessionId);
	}
	return sessionList;
}


void destroy_session_list(Session *sessionList) {
	Session *current = sessionList;
	Session *next = current;
	while(current != NULL) {
		destroy_userlist(current -> usr);
		next = current -> next;
		free(current);
		current = next;
	}
}


#endif