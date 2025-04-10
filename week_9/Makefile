LIBS = -lpthread

all: mysmtp_server mysmtp_client

mysmtp_server: mysmtp_server.c
	gcc -o mysmtp_server mysmtp_server.c $(LIBS)

mysmtp_client: mysmtp_client.c
	gcc -o mysmtp_client mysmtp_client.c

clean:
	rm -f mysmtp_server mysmtp_client
	rm -rf mailbox

.PHONY: all clean