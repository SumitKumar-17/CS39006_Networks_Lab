# Custom Lightweight Discovery Protocol (CLDP)

This project implements a Custom Lightweight Discovery Protocol (CLDP) using raw sockets in C. The protocol is designed for a closed network environment where nodes can announce their presence and query each other for specific application-level metadata.

## Files Included

- `cldp_spec.pdf`: Protocol specification document
- `cldp_server.c`: CLDP server implementation
- `cldp_client.c`: CLDP client implementation
- `README.md`: This file
- `Makefile`: A helper file with commands already written.

## Build Instructions

To compile the CLDP server and client:

```bash
# Compile the server
gcc -o cldp_server cldp_server.c -Wall

# Compile the client
gcc -o cldp_client cldp_client.c -Wall
```

## Run Instructions

Since the implementation uses raw sockets, both programs must be run with root privileges.

### Running the CLDP Server

```bash
sudo ./cldp_server
```

The server will:
1. Listen for incoming CLDP packets
2. Send HELLO messages every 10 seconds to broadcast its presence
3. Process incoming QUERY messages and respond with the requested metadata

### Running the CLDP Client

```bash
sudo ./cldp_client <target_ip>
```

Where:
- `<target_ip>` is the IP address of the server you want to query, or the broadcast address `255.255.255.255` to query all servers on the network.

The client will:
1. Send HELLO messages every 10 seconds
2. Allow you to send queries for hostname, system time, or CPU load
3. Display responses from servers

## Protocol Features

The CLDP implementation supports the following features:

1. **Message Types**:
   - HELLO (0x01): Announces node presence
   - QUERY (0x02): Requests metadata from other nodes
   - RESPONSE (0x03): Returns requested metadata

2. **Metadata Types**:
   - Hostname (0x01): Returns the node's hostname
   - System Time (0x02): Returns the current system time
   - CPU Load (0x03): Returns the CPU load averages

3. **Raw Socket Implementation**:
   - Manual crafting of IP headers (protocol number 253)
   - No transport layer protocols (TCP/UDP)
   - Includes checksum calculation
   - Filters packets by protocol number

## Assumptions and Limitations

1. **Local Network Only**: The protocol is designed for closed network environments and may not work across routers that filter custom IP protocols.

2. **Elevated Privileges**: Both client and server require root/sudo privileges to work with raw sockets.

3. **Linux-Specific Implementation**: The code uses POSIX APIs and is specifically tested on Linux. It may require modifications to work on other operating systems.

4. **IP Protocol Number**: The implementation uses protocol number 253, which could conflict with other custom protocols in your network.

5. **Limited Error Handling**: The implementation provides basic error handling but could be improved for production use.

## Demo Output

Below is a sample output showing the basic operation of the CLDP client and server:

### Server Output:
```
CLDP Server started. Listening for packets...
HELLO message sent
Received CLDP packet from 10.105.55.145, Type: HELLO
Received CLDP packet from 10.105.55.145, Type: QUERY (Type: 1)
Query for hostname received, sending: sumitk
Response sent for query type 1
Received CLDP packet from 127.0.0.1, Type: RESPONSE (Type: 1)
HELLO message sent
Received CLDP packet from 10.105.55.145, Type: HELLO
Received CLDP packet from 10.105.55.145, Type: QUERY (Type: 2)
Query for system time received, sending: 2025-03-31 14:32:23
Response sent for query type 2
Received CLDP packet from 127.0.0.1, Type: RESPONSE (Type: 2)
Received CLDP packet from 10.105.55.145, Type: QUERY (Type: 3)
Query for CPU load received, sending: Load avg: 1.06, 1.50, 1.78
Response sent for query type 3
Received CLDP packet from 127.0.0.1, Type: RESPONSE (Type: 3)
Received CLDP packet from 10.105.55.145, Type: HELLO
```

### Client Output:
```
CLDP Client started. Target: 127.0.0.1

CLDP Client Menu:
1. Send query for hostname
2. Send query for system time
3. Send query for CPU load
4. Exit
Enter your choice: 1
QUERY message sent for Hostname (Transaction ID: 22081)
Response from 127.0.0.1:
  Query Type: Hostname
  Data: sumitk
  Transaction ID: 22081

CLDP Client Menu:
1. Send query for hostname
2. Send query for system time
3. Send query for CPU load
4. Exit
Enter your choice: 2
QUERY message sent for System Time (Transaction ID: 44710)
Received HELLO from 10.105.55.145
Response from 127.0.0.1:
  Query Type: System Time
  Data: 2025-03-31 14:32:23
  Transaction ID: 44710

CLDP Client Menu:
1. Send query for hostname
2. Send query for system time
3. Send query for CPU load
4. Exit
Enter your choice: 3
QUERY message sent for CPU Load (Transaction ID: 30280)
HELLO message sent
Response from 127.0.0.1:
  Query Type: CPU Load
  Data: Load avg: 1.06, 1.50, 1.78
  Transaction ID: 30280

CLDP Client Menu:
1. Send query for hostname
2. Send query for system time
3. Send query for CPU load
4. Exit
Enter your choice: 4
Received HELLO from 10.105.55.145
CLDP Client shut down gracefully.
```
