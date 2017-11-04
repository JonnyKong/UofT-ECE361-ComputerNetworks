#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>

#define BUF_SIZE 1024

#define INVALID_SOCKET - 1
// buffer for send and receive
char buf[BUF_SIZE];
// whether there is a listener thread
bool pthread = false;
// supported commands
const char *LOGIN_CMD = "/login";
const char *LOGOUT_CMD = "/logout";
const char *JOINSESSION_CMD = "/joinsession";
const char *LEAVESESSION_CMD = "/leavesession";
const char *CREATESESSION_CMD = "/createsession";
const char *LIST_CMD = "/list";
const char *QUIT_CMD = "/quit";

void *receive(void *socketfd_void_p) {
	int *socketfd_p = (int *)socketfd_void_p;
	int numbytes;
	for (;;) {
		
		if ((numbytes = recv(*socketfd_p, buf, BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client receive: recv\n");
			return NULL;
		}
		buf[numbytes] = '\0';
		printf("client: received '%s'\n", buf);
	}
	return NULL;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
	}
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void login(char *pch, int *socketfd_p) {
	char *client_id, *password, *server_ip, *server_port;
	// tokenize the command
	pch = strtok(NULL, " ");
	client_id = pch;
	pch = strtok(NULL, " ");
	password = pch;
	pch = strtok(NULL, " ");
	server_ip = pch;
	pch = strtok(NULL, " \n");
	server_port = pch;
	if (client_id == NULL || password == NULL || server_ip == NULL || server_port == NULL) {
		fprintf(stdout, "usage: /login <client_id> <password> <server_ip> <server_port>\n");
		return;
	} else if (*socketfd_p != INVALID_SOCKET) {
		fprintf(stdout, "You can only login to 1 server simutaneously.\n");
		return;
	} else {
		// prepare to connect through TCP protocol
		int rv;
		struct addrinfo hints, *servinfo, *p;
		char s[INET6_ADDRSTRLEN];
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		
		if ((rv = getaddrinfo(server_ip, server_port, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return;
		}
		for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((*socketfd_p = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
				fprintf(stderr ,"client: socket\n");
				continue;
			}
			if (connect(*socketfd_p, p->ai_addr, p->ai_addrlen) == -1) {
				close(*socketfd_p);
				fprintf(stderr, "client: connect\n");
				continue;
			}
			break; 
		}
		if (p == NULL) {
			fprintf(stderr, "client: failed to connect from addrinfo\n");
			return;
		}
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
		printf("client: connecting to %s\n", s);
		freeaddrinfo(servinfo); // all done with this structure

		int numbytes;
		// TODO: content of send
		if ((numbytes = send(*socketfd_p, "login", BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client: send\n");
			*socketfd_p = INVALID_SOCKET;
			return;
		}

		if ((numbytes = recv(*socketfd_p, buf, BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client: recv\n");
			*socketfd_p = INVALID_SOCKET;
			return;
		}
		buf[numbytes] = '\0';
    	printf("client: received '%s'\n", buf);
	}
}

void logout(int *socketfd_p, pthread_t *receive_thread_p) {
	if (*socketfd_p == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} 

	int numbytes;
	// TODO: content of send
	if ((numbytes = send(*socketfd_p, "logout", BUF_SIZE - 1, 0)) == -1) {
		fprintf(stderr, "client: send\n");
		return;
	}
	if (pthread) {
		if (pthread_cancel(*receive_thread_p)) {
			fprintf(stderr, "logout leavesession failure\n");
		} else {
			fprintf(stdout, "logout leavesession success\n");
			pthread = false;
		} 
	}

	close(*socketfd_p);
	*socketfd_p = INVALID_SOCKET;
}

void joinsession(char *pch, int *socketfd_p, pthread_t *receive_thread_p) {
	if (*socketfd_p == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (pthread) {
		fprintf(stdout, "You have already joined a session.\n");
		return;
	} 

	char *session_id;
	pch = strtok(NULL, " ");
	session_id = pch;
	if (session_id == NULL) {
		fprintf(stdout, "usage: /joinsession <session_id>\n");
	} else {
		int numbytes;
		// TODO: content of send
		if ((numbytes = send(*socketfd_p, "joinsession", BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client: send\n");
			return;
		}

		if ((numbytes = recv(*socketfd_p, buf, BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client: recv\n");
			return;
		}
		buf[numbytes] = '\0';
		printf("client: received '%s'\n", buf);

		if(pthread_create(receive_thread_p, NULL, receive, socketfd_p)) {
			fprintf(stderr, "Error creating thread\n");
			return;
		} else {
			fprintf(stdout, "Successfully creating thread\n");
			pthread = true;
		}

	}
}

void leavesession(int socketfd, pthread_t *receive_thread_p) {
	if (socketfd == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (!pthread) {
		fprintf(stdout, "You have not joined a session yet.\n");
		return;
	}

	int numbytes;
	// TODO: content of send
	if ((numbytes = send(socketfd, "leavesession", BUF_SIZE - 1, 0)) == -1) {
		fprintf(stderr, "client: send\n");
		return; 
	}
	if (pthread_cancel(*receive_thread_p)) {
		fprintf(stderr, "leavesession failure\n");
	} else {
		fprintf(stdout, "leavesession success\n");
		pthread = false;
	} 

}

void createsession(char *pch, int *socketfd_p, pthread_t *receive_thread_p) {
	if (*socketfd_p == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (pthread) {
		fprintf(stdout, "You have already joined a session.\n");
		return;
	} 

	char *session_id;
	pch = strtok(NULL, " ");
	session_id = pch;
	if (session_id == NULL) {
		fprintf(stdout, "usage: /joinsession <session_id>\n");
	} else {
		int numbytes;
		// TODO: content of send
		if ((numbytes = send(*socketfd_p, "createsession", BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client: send\n");
			return; 
		}

		if ((numbytes = recv(*socketfd_p, buf, BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client: recv\n");
			return; 
		}
		buf[numbytes] = '\0';
		printf("client: received '%s'\n", buf);

		if(pthread_create(receive_thread_p, NULL, receive, socketfd_p)) {
			fprintf(stderr, "Error creating thread\n");
			return;
		} else {
			fprintf(stdout, "Successfully creating thread\n");
			pthread = true;
		}
	}
}

void list(int socketfd) {
	if (socketfd == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (!pthread) {
		fprintf(stdout, "You have not joined a session yet.\n");
		return;
	}
	int numbytes;
	// TODO: content of send
	if ((numbytes = send(socketfd, "list", BUF_SIZE - 1, 0)) == -1) {
		fprintf(stderr, "client: send\n");
		return; 
	}
}

void send_text(int socketfd) {
	if (socketfd == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (!pthread) {
		fprintf(stdout, "You have not joined a session yet.\n");
		return;
	}

	int numbytes;
	// TODO: content of send
	if ((numbytes = send(socketfd, "text", BUF_SIZE - 1, 0)) == -1) {
		fprintf(stderr, "client: send\n");
		return; 
	}

}

int main() {
	const int LOGIN_CMD_LEN = strlen(LOGIN_CMD);
	const int LOGOUT_CMD_LEN = strlen(LOGOUT_CMD);
	const int JOINSESSION_CMD_LEN = strlen(JOINSESSION_CMD);
	const int LEAVESESSION_CMD_LEN = strlen(LEAVESESSION_CMD);
	const int CREATESESSION_CMD_LEN = strlen(CREATESESSION_CMD);
	const int LIST_CMD_LEN = strlen(LIST_CMD);
	const int QUIT_CMD_LEN = strlen(QUIT_CMD);

	char *pch;
	int socketfd = INVALID_SOCKET;
	pthread_t receive_thread;

	for (;;) {
		fgets(buf, BUF_SIZE - 1, stdin);
		pch = strtok(buf, " ");
		if (strncmp(pch, LOGIN_CMD, LOGIN_CMD_LEN) == 0) {
			login(pch, &socketfd);
		} else if (strncmp(pch, LOGOUT_CMD, LOGOUT_CMD_LEN) == 0) {
			logout(&socketfd, &receive_thread);
		} else if (strncmp(pch, JOINSESSION_CMD, JOINSESSION_CMD_LEN) == 0) {
			joinsession(pch, &socketfd, &receive_thread);
		} else if (strncmp(pch, LEAVESESSION_CMD, LEAVESESSION_CMD_LEN) == 0) {
			leavesession(socketfd, &receive_thread);
		} else if (strncmp(pch, CREATESESSION_CMD, CREATESESSION_CMD_LEN) == 0) {
			createsession(pch, &socketfd, &receive_thread);
		} else if (strncmp(pch, LIST_CMD, LIST_CMD_LEN) == 0) {
			list(socketfd);
		} else if (strncmp(pch, QUIT_CMD, QUIT_CMD_LEN) == 0) {
			logout(&socketfd, &receive_thread);
			break;
		} else {
			send_text(socketfd);
		}
	}
	fprintf(stdout, "You have quit successfully.\n");
	return 0;
}
