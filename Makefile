all: server.o client.o

server.o:
		gcc localclient.c -o client
client.o:
		gcc localserver.c -o server