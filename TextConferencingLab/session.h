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

Session *init_session(Session *sessionList, int sessionId) {
	Session *newSession = malloc(sizeof(Session));
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
	Session *session = isValidSession(sessionList, sessionId);
	assert(session != NULL);

	// Malloc new user
	User *newUsr = malloc(sizeof(User));
	memcpy((void *)newUsr, (void *)usr, sizeof(User));

	// Insert into session list
	session -> usr = add_user(session -> usr, newUsr);

}