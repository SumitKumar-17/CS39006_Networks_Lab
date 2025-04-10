
all: server client

server:  doencfileserver.c
	gcc -Wall -o server doencfileserver.c

client: retrieveencfileclient.c
	gcc -Wall -o client retrieveencfileclient.c

clean:
	rm -f server client *.o *.txt *.enc

.PHONY: all clean