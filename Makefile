CC = gcc
CFLAGS = -Wall -Wextra

server: server.o
	$(CC) $(CFLAGS) -o server server.o

client: client.o
	$(CC) $(CFLAGS) -o client client.o

clean:
	rm server client
