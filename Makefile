CC = gcc
CFLAGS = -Wall -Wextra

server:
	$(CC) $(CFLAGS) -o server server.c

client:
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm server client
