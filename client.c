#include <stdio.h>
#include <stdlib.h>

// Reading device file
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

// Socket
#include <netdb.h>
#include <netinet/in.h>

// Error reporting
#include <string.h>
#include <errno.h>

#define DEVICE "/dev/input/event3"

typedef struct input_event input_event;

int sockfd, r;
struct sockaddr_in server_address;
struct hostent *host;

int connect_to_server(char *hostname, int port)
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return -1;

	host = gethostbyname(hostname);

	bzero((char *)&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	bcopy((char *)host->h_addr, (char *)&server_address.sin_addr.s_addr, host->h_length);
	server_address.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
		return -1;

	return 0;
}

int read_u16(__u16 *target) {
	__u16 tmp;
	r = read(sockfd, &tmp, 2);
	*target = ntohs(tmp);

	if (r < 0)
		printf("Error receiving data: %s\n", strerror(errno));

	return r;
}

int read_s32(__s32 *target) {
	__s32 tmp;
	r = read(sockfd, &tmp, 4);
	*target = ntohl(tmp);

	if (r < 0)
		printf("Error receiving data: %s\n", strerror(errno));

	return r;
}

int is_end_signal(input_event event)
{
	return event.type == 0 && event.code == 0 && event.value == -1;
}


int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Usage: %s <hostname> <port>\n", argv[0]);
		return 1;
	}
	
	int connection = connect_to_server(argv[1], atoi(argv[2]));
	if (connection < 0) {
		printf("Socket error: %s\n", strerror(errno));
		return 1;
	}
	
	printf("Connected to %s:%s.\n", argv[1], argv[2]);

	int file_descriptor = open(DEVICE, O_WRONLY | O_APPEND);
	if (file_descriptor < 0) {
		printf("Cannot open file '%s': %s\n", DEVICE, strerror(errno));
		return 1;
	}

	int running = 1;
	input_event event;
	size_t event_size = sizeof(event);

	while (running) {
		if (read_u16(&event.type) < 1) {
			running = 0;
			break;
		}
		if (read_u16(&event.code) < 1) {
			running = 0;
			break;
		}
		if (read_s32(&event.value) < 1) {
			running = 0;
			break;
		}

		if (is_end_signal(event)) {
#ifdef DEBUG
			printf("End signal was received.\n");
#endif
			running = 0;
			break;
		}

#ifdef DEBUG
		printf("%d %d %d\n", event.type, event.code, event.value);
#endif
		if (write(file_descriptor, &event, event_size) != (int)event_size) {
			printf("Error writeing to device file.\n");
		}
	}

	close(sockfd);
	printf("Stopped.\n");

	return 0;
}
