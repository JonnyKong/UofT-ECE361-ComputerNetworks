#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#include "packet.h"
#include "user.h"
#include "session.h"

#define ULISTFILE "userlist.txt"    // File of username and passwords   
#define INBUF_SIZE 64   			// Input buffer
#define BACKLOG 10	    			// how many pending connections queue will hold

User *userList = NULL; 			// List of all users and passwords
User *userLoggedin = NULL;		// List of users logged in
Session *sessionList;			// List of all sessions created
char inBuf[INBUF_SIZE] = {0};  	// Input buffer
char port[6] = {0}; 			// Store port in string for getaddrinfo
int sessionCnt = 1;			// Session count begins from 1
int userConnectedCnt = 0;			// Count of users connected

// Enforce synchronization
pthread_mutex_t sessionList_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t userLoggedin_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sessionCnt_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t userConnectedCnt_mutex = PTHREAD_MUTEX_INITIALIZER;


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// Subroutine to handle new connections
void *new_client(void *arg) {

	User *newUsr = (User*)arg;
	Session *sessJoined = NULL;	// List of sessions joined
	char buffer[BUF_SIZE];	// Buffer for recv()
	char source[MAX_NAME];	// Username (valid after logged in)
	int bytesSent, bytesRecvd;

	printf("New thread created\n");

	// FSM states
	bool loggedin = 0;

	// The main recv() loop
	while(1) {
		memset(buffer, 0, sizeof(char) * BUF_SIZE);
		if((bytesRecvd = recv(newUsr -> sockfd, buffer, BUF_SIZE - 1, 0)) == -1) {
			perror("error recv\n");
			exit(1);
		}
		buffer[bytesRecvd] = '\0';
		Packet pktRecv;		// Convert data into a new packet
		Packet pktSend;		// Packet to send
		bool toSend = 0;	// Whether to send pktSend after this loop
		printf("Message received: \"%s\"\n", buffer);

		stringToPacket(buffer, &pktRecv);
		memset(&pktSend, 0, sizeof(Packet));


		/************ Finite State Machine *************/

		// Can exit anytime
		if(pktRecv.type == EXIT) {
			// Thread exits, no ACK packet sent, close socket
			close(newUsr -> sockfd);

			// Leave all session before user exits
			for(Session *cur = sessJoined; cur != NULL; cur = cur -> next) {
				pthread_mutex_lock(&sessionList_mutex);
				sessionList = remove_session(sessionList, cur -> sessionId);
				pthread_mutex_unlock(&sessionList_mutex);
			}

			// remove private session list, free memory
			destroy_session_list(sessionList);
			free(newUsr);

			// Decrement userConnectedCnt
			pthread_mutex_lock(&userConnectedCnt_mutex);
			--userConnectedCnt;
			pthread_mutex_unlock(&userConnectedCnt_mutex);
			
			if(loggedin) {
				printf("User %s exiting...\n", newUsr -> uname);
			} else {
				printf("User exiting...\n");
			}
			return NULL;
		}

		
		// Before login, the user can only login or exit
		if(loggedin == 0) {	

			if(pktRecv.type == LOGIN) {	
				// Parse and check username and password
				int cursor = 0;
				while(pktRecv.data[cursor] != ' ') ++cursor;
				memcpy(newUsr -> uname, (char *)(pktRecv.data), cursor);
				strcpy(newUsr -> pwd, (char *)(pktRecv.data + cursor + 1));
				bool vldusr = is_valid_user(userList, newUsr);
				// Clear user password for safety
				memset(newUsr -> pwd, 0, PWDLEN);
				
				if(vldusr) {
					pktSend.type = LO_ACK;
					toSend = 1;
					loggedin = 1;
					
					// Add user to userLoggedin list
					User *tmp = malloc(sizeof(User));
					memcpy(tmp, newUsr, sizeof(User));
					pthread_mutex_lock(&userLoggedin_mutex);
					userLoggedin = add_user(userLoggedin, tmp);
					pthread_mutex_unlock(&userLoggedin_mutex);

					printf("User %s: Successfully logged in...\n", newUsr -> uname);

				} else {
					pktSend.type = LO_NAK;
					toSend = 1;
					// Respond with reason for failure
					if(in_list(userList, newUsr)) {	
						strcpy((char *)(pktSend.data), "Wrong password");
					} else {
						strcpy((char *)(pktSend.data), "User not registered");
					}
					printf("Log in failed from anonymous user\n");

					// Clear local user data for new login request
					memset(newUsr -> uname, 0, UNAMELEN);
				}
			} else {
				pktSend.type = LO_NAK;
				toSend = 1;
				strcpy((char *)(pktSend.data), "Please log in first");
			}
		}


		// User logged in, join session
		else if(pktRecv.type == JOIN) {
			int sessionId = atoi((char *)(pktRecv.data));
			printf("User %s: Trying to join session %d...\n", newUsr -> uname, sessionId);

			// Fails if session not exists
			if(isValidSession(sessionList, sessionId) == NULL) {
				pktSend.type = JN_NAK;
				toSend = 1;
				int cursor = sprintf((char *)(pktSend.data), "%d", sessionId);
				strcpy((char *)(pktSend.data + cursor), " Session not exist");
				printf("User %s: Failed to join session %d\n", newUsr -> uname, sessionId);
			} 
			// Fails if already joined session
			else if(inSession(sessionList, sessionId, newUsr)) {
				pktSend.type = JN_NAK;
				toSend = 1;
				int cursor = sprintf((char *)(pktSend.data), "%d", sessionId);
				strcpy((char *)(pktSend.data + cursor), " Session already joined");
				printf("User %s: Failed to join session %d\n", newUsr -> uname, sessionId);
			}
			// Success join session
			else {
				// Update pktSend JN_ACK
				pktSend.type = JN_ACK;
				toSend = 1;
				sprintf((char *)(pktSend.data), "%d", sessionId);

				// Update global sessionList
				pthread_mutex_lock(&sessionList_mutex);
				sessionList = join_session(sessionList, sessionId, newUsr);
				pthread_mutex_unlock(&sessionList_mutex);

				// Update private sessJoined
				sessJoined = init_session(sessJoined, sessionId);

				printf("User %s: Succeeded join session %d\n", newUsr -> uname, sessionId);
			}
		}


		// User logged in, leave session, no reply
		else if(pktRecv.type == LEAVE_SESS) {
			printf("User %s trying to leave all sessions...:\n", newUsr -> uname);
			
			// Iterate until all session left
			while(sessJoined != NULL) {
				int curSessId = sessJoined -> sessionId;
				// Free private sessJoined
				Session *cur = sessJoined;
				sessJoined = sessJoined -> next;
				free(cur);
				// Free global sessionList
				pthread_mutex_lock(&sessionList_mutex);
				sessionList = leave_session(sessionList, curSessId, newUsr);
				pthread_mutex_unlock(&sessionList_mutex);

				printf("\tUser %s: Left session %d\n", newUsr -> uname, curSessId);
			}
		}


		// User create new session 
		else if(pktRecv.type == NEW_SESS) {
			printf("User %s: Trying to create new session...:\n", newUsr -> uname);

			// Update global session_list
			pthread_mutex_lock(&sessionList_mutex);
			sessionList = init_session(sessionList, sessionCnt);
			pthread_mutex_unlock(&sessionList_mutex);

			// User join just created session
			sessJoined = init_session(sessJoined, sessionCnt);

			// Update pktSend NS_ACK
			pktSend.type = NS_ACK;
			toSend = 1;
			sprintf((char *)(pktSend.data), "%d", sessionCnt);

			// Update sessionCnt
			pthread_mutex_lock(&sessionCnt_mutex);
			++sessionCnt;
			pthread_mutex_unlock(&sessionCnt_mutex);

			printf("\tUser %s: Successfully created session %d\n", newUsr -> uname, sessionCnt - 1);
			
		}


		// User send message
		else if(pktRecv.type == MESSAGE) {
			printf("User %s: Sending message...\n", newUsr -> uname);

			// Prepare message to be sent
			memset(&pktSend, 0, sizeof(Packet));
			pktSend.type = MESSAGE;
			strcpy((char *)(pktSend.source), newUsr -> uname);
			strcpy((char *)(pktSend.data), (char *)(pktRecv.data));
			pktSend.size = strlen((char *)(pktSend.data));
			
			// Use recv() buffer
			memset(recv, 0, sizeof(char) * BUF_SIZE);
			packetToString(&pktSend, buffer);

			// Send though local session list
			for(Session *cur = sessJoined; cur != NULL; cur = cur -> next) {
				/* Send this message to all users in this session.
				 * User may receive duplicate messages.
				 */
				// Find corresponding session in global sessionList
				Session *sessToSend = isValidSession(sessionList, cur -> sessionId);
				assert(sessToSend != NULL);
				for(User *usr = sessToSend -> usr; usr != NULL; usr = usr -> next) {
					if((bytesRecvd = send(usr -> sockfd, buffer, BUF_SIZE - 1, 0)) == -1) {
						perror("error send\n");
						exit(1);
					}
				}
			}
			toSend = 0;
		}


		// Respond user query
		else if(pktRecv.type == QUERY) {
			
			printf("User %s: Making query...\n", newUsr -> uname);

			int cursor = 0;
			pktSend.type = QU_ACK;
			toSend = 1;
			/* Interate thorugh all sessions, output format:
			 * Session1: user1 user2 user3
			 * Session2: user5 user3 user6
			 */
			for(Session *curSess = sessionList; curSess != NULL; curSess = curSess -> next) {
				cursor += sprintf((char *)(pktSend.data) + cursor, "Session %d:", curSess -> sessionId);
				for(User *usr = curSess -> usr; usr != NULL; usr = usr -> next) {
					cursor += sprintf((char *)(pktSend.data) + cursor, "\t%s", usr -> uname);
				}
				// Add carrige return after each session
				pktSend.data[cursor++] = '\n';
			}

			printf("Query Result:\n%s", pktSend.data);
		}


		if(toSend) {
			// Add source and size for pktSend and send packet
			memcpy(pktSend.source, newUsr -> uname, UNAMELEN);
			pktSend.size = strlen((char *)(pktSend.data));

			if((bytesRecvd = send(newUsr -> sockfd, buffer, BUF_SIZE - 1, 0)) == -1) {
				perror("error send\n");
			}
		}
	}

}


int main() {
    // Get user input
    memset(inBuf, 0, INBUF_SIZE * sizeof(char));
	fgets(inBuf, INBUF_SIZE, stdin);
    char *pch;
    if((pch = strstr(inBuf, "server ")) != NULL) {
		memcpy(port, pch + 7, sizeof(char) * (strlen(pch + 7) - 1));
    } else {
        perror("Usage: server -<TCP port number to listen on>\n");
        exit(1);
    }
    printf("Server: starting at port %s...\n", port);


    // Load userlist at startup
    FILE *fp;
    if((fp = fopen(ULISTFILE, "r")) == NULL) {
        fprintf(stderr, "Can't open input file %s\n", ULISTFILE);
    }
    userList = init_userlist(fp);
	fclose(fp);
	printf("Server: Userdata loaded\n");


    // Setup server
	int sockfd;     // listen on sock_fd, new connection on new_fd
	// int new_fd;		
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof(hints));
	// hints.ai_family = AF_UNSPEC;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;    // Use TCP
	hints.ai_flags = AI_PASSIVE;        // use my IP
	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("Server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("Server: bind");
			continue;
		}
		break;
	}
	freeaddrinfo(servinfo); // all done with this structure
	if (p == NULL)  {
		fprintf(stderr, "Server: failed to bind\n");
		exit(1);
    }
    

    // Listen on incoming port
    if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	printf("Server: waiting for connections...\n");

    
	// main accept() loop
	while(1) {

		// Accept new incoming connections 
		User *newUsr = calloc(sizeof(User), 1);
		sin_size = sizeof(their_addr);
		newUsr -> sockfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (newUsr -> sockfd == -1) {
			perror("accept");
			continue;
		}
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));
		printf("Server: got connection from %s\n", s);

		// Increment userConnectedCnt
		pthread_mutex_lock(&userConnectedCnt_mutex);
		++userConnectedCnt;
		pthread_mutex_unlock(&userConnectedCnt_mutex);

		// Create new thread to handle the new socket
		pthread_create(&(newUsr -> p), NULL, new_client, (void *)newUsr);

		// Temp: server quites as last user leaves session
		if(userConnectedCnt == 0) {
			printf("All user have left session, server terminating...\n");
			break;
		}
	}


	// Free global memory on exit
	destroy_userlist(userList);
	destroy_userlist(userLoggedin);
	destroy_session_list(sessionList);
	printf("Server Terminated\n");
    return 0;
}