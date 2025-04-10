CC = gcc
CFLAGS = -Wall -Wextra -pedantic 
DEBUG = -g

all: server client

server: task_queue_server.c
	$(CC) $(CFLAGS) $(DEBUG) -o task_queue_server task_queue_server.c

client: worker_client.c
	$(CC) $(CFLAGS) $(DEBUG) -o worker_client worker_client.c

test: 
	./test_script.sh

clean:
	rm -f task_queue_server worker_client *.o
	rm -rf test_logs

.PHONY: all clean