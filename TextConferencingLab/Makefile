TARGET = server client
DEPS = packet.h user.h

all:  ${TARGET}

server: server.c ${DEPS}
	gcc -o server server.c

client: client.c ${DEPS}
	gcc -o client client.c

clean:
	rm -f ${TARGET}
