#include <stdio.h>
#include <signal.h>

// Reading device file
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

// Error reporting
#include <string.h>
#include <errno.h>

typedef struct input_event input_event;
static volatile int running = 1;

static void stop(int n)
{
	running = 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s <device_file>\n", argv[0]);
		return 1;
	}
	
	int file_descriptor = open(argv[1], O_RDONLY);
	if (file_descriptor == -1) {
		printf("Cannot open '%s' for reading: %s\n", argv[1], strerror(errno));
		return 1;
	}
	
	signal(SIGINT, stop);
	printf("Handling keyboard input.\n");
	
	input_event event;
	size_t event_size = sizeof(input_event);
	
	while (read(file_descriptor, &event, event_size) > 0 && running) {
		printf("Event: time %ld.%06ld, type %d, code %d, value %d\n", event.time.tv_sec, event.time.tv_usec, event.type, event.code, event.value);
	}
	
	close(file_descriptor);
	printf("Exiting.\n");
	
	return 0;
}
