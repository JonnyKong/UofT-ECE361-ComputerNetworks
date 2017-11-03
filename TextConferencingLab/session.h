#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

// #include "user.h"

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
Session *init_session(Session *sessionList, int sessionId) {
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

void join_session(Session *sessionList, int sessionId, const User *usr) {
	// Check session exists outside & Find current session
	Session *cur = isValidSession(sessionList, sessionId);
	assert(cur != NULL);

	// Malloc new user
	User *newUsr = malloc(sizeof(User), 1);
	memcpy((void *)newUsr, (void *)usr, sizeof(User));

	// Insert into session list
	cur -> usr = add_user(cur -> usr, newUsr);
}


Session *leave_session(Session *sessionList, int sessionId, const User *usr) {
	// Check session exists outside & Find current session
	Session *cur isValidSession(sessionList, sessionId);
	assert(cur != NULL);

	// Remove user from session
	assert(in_list(cur -> usr, usr));
	remove_user(cur -> usr, usr -> uname);

	// If last user in session, the terminate session
	if(cur -> usr == NULL) {
		// Search for this session again from sessionList
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
			while(cur != NULl) {
				if(cur -> sessionId == sessionId) {
					prev -> next = cur -> next;
					free(cur);
					break;
				} else {
					prev = cur;
					cur = cur -> next;
				}
			}
		}
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