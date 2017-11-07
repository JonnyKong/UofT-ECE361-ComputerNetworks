#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/select.h>

#include "packet.h"
#define INVALID_SOCKET -1
#define INVALID_SESSION -1
#define MAX_SESSION 3
// buffer for send and receive
char buf[BUF_SIZE];
// which sessions user is in
int insessions[MAX_SESSION];
// the session to send message to
int insession; 
// number of sessions currently in
int session_count;
// whether in an invitation
bool invit = false;
// mutex for invitation boolean
pthread_mutex_t invit_mutex = PTHREAD_MUTEX_INITIALIZER;
// invite session
char invit_session[MAX_DATA];
// whoami
char username[MAX_NAME] = {0};

// supported commands
const char *LOGIN_CMD = "/login";
const char *LOGOUT_CMD = "/logout";
const char *JOINSESSION_CMD = "/joinsession";
const char *LEAVESESSION_CMD = "/leavesession";
const char *CREATESESSION_CMD = "/createsession";
const char *LIST_CMD = "/list";
const char *QUIT_CMD = "/quit";
const char *SWITCHSESSION_CMD = "/switchsession";
const char *INVITE_CMD = "/invite";

void *receive(void *socketfd_void_p) {
	// receive may get: JN_ACK, JN_NAC, NS_ACK, QU_ACK, MESSAGE
  // only LOGIN packet is not listened by receive
  int *socketfd_p = (int *)socketfd_void_p;
	int numbytes;
	Packet packet;
  for (;;) {	
		if ((numbytes = recv(*socketfd_p, buf, BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client receive: recv\n");
			return NULL;
		}
    buf[numbytes] = 0;
    //fprintf(stdout, "buf: %s\n", buf);
		stringToPacket(buf, &packet);
    //fprintf(stdout, "packet.type: %d, packet.data: %s\n", packet.type, packet.data);
    if (packet.type == JN_ACK) {
      fprintf(stdout, "Successfully joined session %s.\n", packet.data);
      insession = atoi(packet.data);
      insessions[session_count] = insession;
      session_count++;
    } else if (packet.type == JN_NAK) {
      fprintf(stdout, "Join failure. Detail: %s\n", packet.data);
      insession = INVALID_SESSION;
    } else if (packet.type == NS_ACK) {
      fprintf(stdout, "Successfully created and joined session %s.\n", packet.data);
      insession = atoi(packet.data);
      insessions[session_count] = insession;
      session_count++;
    } else if (packet.type == QU_ACK) {
      if (insession != INVALID_SESSION) {
        fprintf(stdout, "You are currently engaging with session #%d.\n", insession);
      } else {
        fprintf(stdout, "You are not engaging with any session.\n");
      }
      fprintf(stdout, "User id\t\tSession ids\n%s", packet.data);
    } else if (packet.type == MESSAGE){   
      fprintf(stdout, "\n--------%s: %s\n", packet.source, packet.data);
    } else if (packet.type == INVIT) { 
      pthread_mutex_lock(&invit_mutex);
      invit = true;
      fprintf(stdout, "%s is inviting you to session #%s, do you accept? (y/n)\n", packet.source, packet.data); 
      strncpy(invit_session, packet.data, MAX_DATA - 1);
      pthread_mutex_unlock(&invit_mutex);
    } else {
      fprintf(stdout, "Unexpected packet received: type %d, data %s\n",
          packet.type, packet.data);
    }
    // solve the ghost-newliner 
    fprintf(stdout, "%s:", username);
    fflush(stdout);
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

void login(char *pch, int *socketfd_p, pthread_t *receive_thread_p) {
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
			close(*socketfd_p);
      *socketfd_p = INVALID_SOCKET;
      fprintf(stderr, "client: failed to connect from addrinfo\n");
			return;
		}
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
		printf("client: connecting to %s\n", s);
		freeaddrinfo(servinfo); // all done with this structure

		int numbytes;
    Packet packet;
    packet.type = LOGIN;
    strncpy(packet.source, client_id, MAX_NAME - 1);
    strncpy(packet.data, password, MAX_DATA - 1);
    packet.size = strlen(packet.data);
    packetToString(&packet, buf);
		if ((numbytes = send(*socketfd_p, buf, BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client: send\n");
			close(*socketfd_p);
      *socketfd_p = INVALID_SOCKET;
			return;
		}

		if ((numbytes = recv(*socketfd_p, buf, BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client: recv\n");
			close(*socketfd_p);
      *socketfd_p = INVALID_SOCKET;
			return;
		}
    buf[numbytes] = 0; 
    stringToPacket(buf, &packet);
    if (packet.type == LO_ACK && 
        pthread_create(receive_thread_p, NULL, receive, socketfd_p) == 0) {
      fprintf(stdout, "login successful.\n");  
      strncpy(username, packet.source, MAX_NAME - 1);
    } else if (packet.type == LO_NAK) {
      fprintf(stdout, "login failed. Detail: %s\n", packet.data);
      close(*socketfd_p);
      *socketfd_p = INVALID_SOCKET;
			return;
    } else {
      fprintf(stdout, "Unexpected packet received: type %d, data %s\n", 
          packet.type, packet.data);
      close(*socketfd_p);
      *socketfd_p = INVALID_SOCKET;
			return;
    } 
	}
}

void logout(int *socketfd_p, pthread_t *receive_thread_p) {
	if (*socketfd_p == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} 

	int numbytes;
  Packet packet;
  packet.type = EXIT;
  packet.size = 0;
  packetToString(&packet, buf);
	
	if ((numbytes = send(*socketfd_p, buf, BUF_SIZE - 1, 0)) == -1) {
		fprintf(stderr, "client: send\n");
		return;
	}	
	if (pthread_cancel(*receive_thread_p)) {
			fprintf(stderr, "logout leavesession failure\n");
	} else {
			fprintf(stdout, "logout leavesession success\n");	
	} 
  insession = INVALID_SESSION;
  for (int i = 0; i < MAX_SESSION; i++) {
    insessions[i] = INVALID_SESSION;
  }
  session_count = 0;
	username[0] = 0;
  close(*socketfd_p);
	*socketfd_p = INVALID_SOCKET;
}

void joinsession(char *pch, int *socketfd_p) {
	if (*socketfd_p == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (session_count >= MAX_SESSION) {
    fprintf(stdout, "You have already joined maximal number of sessions.\n");
    return;
  } 

	char *session_id;
	pch = strtok(NULL, " ");
	session_id = pch;
	if (session_id == NULL) {
		fprintf(stdout, "usage: /joinsession <session_id>\n");
	} else {
		int numbytes;
    Packet packet;
    packet.type = JOIN;
    strncpy(packet.data, session_id, MAX_DATA - 1);
    packet.size = strlen(packet.data);
    packetToString(&packet, buf);
		if ((numbytes = send(*socketfd_p, buf, BUF_SIZE - 1, 0)) == -1) {
			fprintf(stderr, "client: send\n");
			return;
		}	
	}
}

void leavesession(int socketfd, char *pch) {
  pch = strtok(NULL, " ");
  if (pch == NULL) {
    fprintf(stdout, "usage: /leavesession <session_id>\n");
    return;
  }
  int session_id = atoi(pch);
  printf("leaving session: %d\n", session_id);
	if (socketfd == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (insession == INVALID_SESSION) {
    fprintf(stdout, "You have not joined a session yet.\n");
    return;
  }
  bool valid = false;
  int idx = 0;
  for (; idx < MAX_SESSION; idx++) {
    if (session_id == insessions[idx]) { 
      valid = true;
      break;
    }
  }
  if (valid) {
    int numbytes;
    Packet packet;
    packet.type = LEAVE_SESS;
    strncpy(packet.source, pch, MAX_DATA);
    packet.size = 0;
    packetToString(&packet, buf);
    if ((numbytes = send(socketfd, buf, BUF_SIZE - 1, 0)) == -1) {
      fprintf(stderr, "client: send\n");
      return; 
    }
    session_count--;
    fprintf(stdout, "You have left session %d.\n", session_id);
    insessions[idx] = INVALID_SESSION;
    if (insession == session_id) {
      insession = INVALID_SESSION;
    }
  } else {
    fprintf(stdout, "You are not currently in session %d.\n", session_id);
  }
}

void createsession(int socketfd) {
	if (socketfd == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (session_count >= MAX_SESSION) {
    fprintf(stdout, "You have already joined maximal number of sessions.\n");
    return;
  }
	
  int numbytes;
  Packet packet;
  packet.type = NEW_SESS;
  packet.size = 0;
  packetToString(&packet, buf);		
  if ((numbytes = send(socketfd, buf, BUF_SIZE - 1, 0)) == -1) {
    fprintf(stderr, "client: send\n");
    return; 
  } 
}

void list(int socketfd) {
	if (socketfd == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} 
	int numbytes;
  Packet packet;
  packet.type = QUERY;
  packet.size = 0;
  packetToString(&packet, buf);	
	if ((numbytes = send(socketfd, buf, BUF_SIZE - 1, 0)) == -1) {
		fprintf(stderr, "client: send\n");
		return; 
	}
}

void send_text(int socketfd) {
  if (socketfd == INVALID_SOCKET) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (insession == INVALID_SESSION) {
    fprintf(stdout, "You have not been engaged in any session yet.\n");
    return;
  } 

	int numbytes;
  Packet packet;
  packet.type = MESSAGE;
  numbytes = snprintf(packet.source, MAX_DATA - 1, "%d", insession);  
  strncpy(packet.data, buf, MAX_DATA - 1);
  packet.size = strlen(packet.data); 
  packetToString(&packet, buf);	
	if ((numbytes = send(socketfd, buf, BUF_SIZE - 1, 0)) == -1) {
		fprintf(stderr, "client: send\n");
		return; 
	}
}

void switchsession(char *pch) {
  pch = strtok(NULL, " ");
  if (pch == NULL) {
    fprintf(stdout, "usage: /switchsession <session_id>\n");
    return;
  }
  int session_id = atoi(pch);
  if (session_id < 0) {
    insession = INVALID_SESSION;
    fprintf(stdout, "You have disengaged from any session.\n");
    return;
  }
  for (int i = 0; i < MAX_SESSION; i++) {
    if (session_id == insessions[i]) {
      insession = session_id;
      fprintf(stdout, "Switched to session %d.\n", session_id);
      return;
    }
  } 
  fprintf(stdout, "You have not joined session %d yet.\n", session_id); 
}

void invite(int socketfd, char *pch) {
  char *user_id;
  char *session_id;
  pch = strtok(NULL, " ");
  user_id = pch;
  pch = strtok(NULL, " ");
  session_id = pch;
  if (user_id == NULL || session_id == NULL) {
    fprintf(stdout, "usage: /invite <user_id> <session_id>\n");
  }
  bool valid = false;
  for (int i = 0; i < MAX_SESSION; i++) {
    if (insessions[i] == atoi(session_id)) {
      valid = true;
      break;
    } 
  }
  if (valid) {
    int numbytes;
    Packet packet;
    packet.type = INVIT; 
    strncpy(packet.source, user_id, MAX_NAME - 1);
    strncpy(packet.data, session_id, MAX_DATA - 1);
    packet.size = strlen(packet.data);
    packetToString(&packet, buf);	
    if ((numbytes = send(socketfd, buf, BUF_SIZE - 1, 0)) == -1) {
      fprintf(stderr, "client: send\n");
      return; 
    }
  } else {
    fprintf(stdout, "You are not a member in session %s.\n", session_id);
  }
}

int main() {
	char *pch;
	int toklen;
	int socketfd = INVALID_SOCKET;
	for (int i = 0; i < MAX_SESSION; i++) {
    insessions[i] = INVALID_SESSION;
  }
  insession = INVALID_SESSION;
  session_count = 0;
  pthread_t receive_thread;
  
	for (;;) {
		fprintf(stdout, "%s:", username);
    fgets(buf, BUF_SIZE - 1, stdin);
		// set the input of newliner as end of input
	  buf[strcspn(buf, "\n")] = 0;
    pch = buf;
    while (*pch == ' ') pch++;
    if (*pch == 0) {
      // ignore empty command
      continue;
    } else if (invit) {
      pthread_mutex_lock(&invit_mutex);
      if (buf[0] == 'y') {
        sprintf(buf, "/joinsession %s", invit_session);
      } 
      pch = strtok(buf, " ");
      joinsession(pch, &socketfd);
      invit = false;
      pthread_mutex_unlock(&invit_mutex);
      continue;
    } 
		pch = strtok(buf, " ");
		toklen = strlen(pch);
		if (strcmp(pch, LOGIN_CMD) == 0) {
			login(pch, &socketfd, &receive_thread);
		} else if (strcmp(pch, LOGOUT_CMD) == 0) {
			logout(&socketfd, &receive_thread);
		} else if (strcmp(pch, JOINSESSION_CMD) == 0) {
			joinsession(pch, &socketfd);
		} else if (strcmp(pch, LEAVESESSION_CMD) == 0) {
			leavesession(socketfd, pch);
		} else if (strcmp(pch, CREATESESSION_CMD) == 0) {
			createsession(socketfd);
		} else if (strcmp(pch, LIST_CMD) == 0) {
			list(socketfd);
		} else if (strcmp(pch, QUIT_CMD) == 0) {
			logout(&socketfd, &receive_thread);
			break;
		} else if (strcmp(pch, SWITCHSESSION_CMD) == 0) {
      switchsession(pch);
    } else if (strcmp(pch, INVITE_CMD) == 0) {
      invite(socketfd, pch);
    } else {
			// restore the buffer to send the original text
			buf[toklen] = ' ';
      send_text(socketfd);
		}
	}
	fprintf(stdout, "You have quit successfully.\n");
	return 0;
}
