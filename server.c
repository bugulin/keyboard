#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// Reading device file
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

// Socket
#include <netdb.h>
#include <sys/socket.h>

// Error reporting
#include <string.h>
#include <errno.h>

typedef struct input_event input_event;

int server, client;
struct sockaddr_in server_address, client_address;
socklen_t client_size;
volatile int running = 1;

void stop(int n)
{
	running = 0;
}

int init_socket(int port)
{
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0)
		return -1;
	
	bzero((char *)&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port);

	if (bind(server, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
		return -1;

	return 0;
}

void start_server(void)
{
	listen(server, 1);
	client_size = sizeof(client_address);
}

int send_u16(__u16 number)
{
	__u16 tmp = htons(number);
	return send(client, &tmp, sizeof(tmp), MSG_NOSIGNAL);
}

int send_s32(__s32 number)
{
	__s32 tmp = htonl(number);
	return send(client, &tmp, sizeof(tmp), MSG_NOSIGNAL);
}

void send_end_signal(void)
{
#ifdef DEBUG
	printf("Sending end signal.\n");
#endif
	send_u16(0);
	send_u16(0);
	send_s32(-1);
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Usage: %s <device_file> <port>\n", argv[0]);
		return 1;
	}
	
	int file_descriptor = open(argv[1], O_RDONLY);
	if (file_descriptor < 0) {
		printf("Cannot open '%s' for reading: %s\n", argv[1], strerror(errno));
		return 1;
	}

	int server_socket = init_socket(atoi(argv[2]));
	if (server_socket < 0) {
		printf("Socket error: %s\n", strerror(errno));
		return 1;
	}
	
	start_server();
	printf("Waiting for a client.\n");
	client = accept(server, (struct sockaddr *)&client_address, &client_size);
	if (client < 0) {
		printf("Error on accepting a connection.\n");
		return 1;
	}
	
	signal(SIGINT, stop);
	
	input_event event;
	size_t event_size = sizeof(input_event);
	
	while (read(file_descriptor, &event, event_size) > 0 && running) {
#ifdef DEBUG
		printf("Event: time %ld.%06ld, type %d, code %d, value %d\n", event.time.tv_sec, event.time.tv_usec, event.type, event.code, event.value);
#endif
		
		if (send_u16(event.type) < 0) {
			printf("Error sending data (event.type): %s\n", strerror(errno));
			running = 0;
			break;
		}
		if (send_u16(event.code) < 0) {
			printf("Error sending data (event.code): %s\n", strerror(errno));
			running = 0;
			break;
		}
		if (send_s32(event.value) < 0) {
			printf("Error sending data (event.value): %s\n", strerror(errno));
			running = 0;
			break;
		}
	}

	send_end_signal();
	usleep(100000); // Waiting for clients to end connection.

	close(file_descriptor);
	close(client);
	close(server);
	printf("Exiting.\n");
	
	return 0;
}
