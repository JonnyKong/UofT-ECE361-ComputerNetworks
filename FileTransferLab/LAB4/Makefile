TARGET = server deliver

all: ${TARGET}
server: server.c packet.h
	gcc server.c -o server
deliver: deliver.c packet.h
	gcc deliver.c -o deliver
clean:  
	rm -f ${TARGET}