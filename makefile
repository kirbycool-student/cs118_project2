CC = gcc
CFLAGS = 
SERVER_OBJS = server.o
CLIENT_OBJS = client.o

all: server client

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o server
server.o: server.c
	$(CC) $(CFLAGS) -c server.c -o server.o

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o client
client.o: client.c
	$(CC) $(CFLAGS) -c client.c -o client.o


clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) server client
