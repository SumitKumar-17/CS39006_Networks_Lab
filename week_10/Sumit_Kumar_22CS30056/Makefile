all: server client

server: cldp_server.c 
	gcc -o cldp_server cldp_server.c -Wall
	sudo ./cldp_server

client: cldp_client.c 
	gcc -o cldp_client cldp_client.c -Wall
	sudo ./cldp_client 127.0.0.1

clean:
	rm -rf cldp_client cldp_server