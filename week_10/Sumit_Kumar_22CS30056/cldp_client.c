/*
=====================================
Assignment 7 Submission
Name: Sumit Kumar
Roll number: 22CS30056
=====================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <netdb.h>

// Custom protocol number
#define CLDP_PROTOCOL 253  

#define MAX_PACKET_SIZE 1024
#define HELLO_TYPE 0x01
#define QUERY_TYPE 0x02
#define RESPONSE_TYPE 0x03

// Metadata query types
#define HOSTNAME_QUERY 0x01
#define SYSTEM_TIME_QUERY 0x02
#define CPU_LOAD_QUERY 0x03

// CLDP header structure
struct cldp_header {
    uint8_t msg_type;
    uint8_t query_type;
    uint16_t payload_length;
    uint16_t transaction_id;
    uint16_t reserved;
};

volatile int running = 1;
volatile int query_active = 0;
uint16_t current_transaction_id = 0;

void handle_signal(int signo) {
    running = 0;
    printf("\nShutting down the CLDP client...\n");
}

// Calculate IP header checksum
unsigned short calculate_checksum(unsigned short *addr, int len) {
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    // Adding 16-bit words
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    // Add left-over byte, if any
    if (nleft == 1) {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    // Fold 32-bit sum to 16 bits
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

// Send HELLO message
void send_hello_message(int sock, struct sockaddr_in *dest_addr) {
    struct iphdr ip_header;
    struct cldp_header cldp_header;
    char packet[MAX_PACKET_SIZE];
    
    // Set IP header
    memset(&ip_header, 0, sizeof(ip_header));
    ip_header.ihl = 5;
    ip_header.version = 4;
    ip_header.tos = 0;
    ip_header.tot_len = sizeof(struct iphdr) + sizeof(struct cldp_header);
    ip_header.id = htons(rand() % 65535);
    ip_header.frag_off = 0;
    ip_header.ttl = 64;
    ip_header.protocol = CLDP_PROTOCOL;
    ip_header.check = 0;
    
    // Get local IP address
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    struct hostent *host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        perror("gethostbyname");
        return;
    }
    
    struct in_addr **addr_list = (struct in_addr **)host_entry->h_addr_list;
    ip_header.saddr = inet_addr(inet_ntoa(*addr_list[0]));
    
    // Set destination address
    ip_header.daddr = dest_addr->sin_addr.s_addr;
    
    // Calculate IP header checksum
    ip_header.check = calculate_checksum((unsigned short *)&ip_header, sizeof(struct iphdr));
    
    // Set CLDP header
    memset(&cldp_header, 0, sizeof(cldp_header));
    cldp_header.msg_type = HELLO_TYPE;
    // No specific query for HELLO
    cldp_header.query_type = 0;  
    cldp_header.payload_length = 0;
    cldp_header.transaction_id = htons(rand() % 65535);
    cldp_header.reserved = 0;
    
    // Assemble the packet
    memcpy(packet, &ip_header, sizeof(ip_header));
    memcpy(packet + sizeof(ip_header), &cldp_header, sizeof(cldp_header));
    
    // Send the packet
    if (sendto(sock, packet, ip_header.tot_len, 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr)) < 0) {
        perror("sendto failed");
    } else {
        printf("HELLO message sent\n");
    }
}

// Send a query message
void send_query(int sock, struct sockaddr_in *dest_addr, uint8_t query_type) {
    struct iphdr ip_header;
    struct cldp_header cldp_header;
    char packet[MAX_PACKET_SIZE];
    
    // Set IP header
    memset(&ip_header, 0, sizeof(ip_header));
    ip_header.ihl = 5;
    ip_header.version = 4;
    ip_header.tos = 0;
    ip_header.tot_len = sizeof(struct iphdr) + sizeof(struct cldp_header);
    ip_header.id = htons(rand() % 65535);
    ip_header.frag_off = 0;
    ip_header.ttl = 64;
    ip_header.protocol = CLDP_PROTOCOL;
    ip_header.check = 0;
    
    // Get local IP address
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    struct hostent *host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        perror("gethostbyname");
        return;
    }
    
    struct in_addr **addr_list = (struct in_addr **)host_entry->h_addr_list;
    ip_header.saddr = inet_addr(inet_ntoa(*addr_list[0]));
    
    // Set destination address
    ip_header.daddr = dest_addr->sin_addr.s_addr;
    
    // Calculate IP header checksum
    ip_header.check = calculate_checksum((unsigned short *)&ip_header, sizeof(struct iphdr));
    
    // Set CLDP header
    memset(&cldp_header, 0, sizeof(cldp_header));
    cldp_header.msg_type = QUERY_TYPE;
    cldp_header.query_type = query_type;
    cldp_header.payload_length = 0;
    
    // Generate a new transaction ID
    current_transaction_id = rand() % 65535;
    cldp_header.transaction_id = htons(current_transaction_id);
    cldp_header.reserved = 0;
    
    // Assemble the packet
    memcpy(packet, &ip_header, sizeof(ip_header));
    memcpy(packet + sizeof(ip_header), &cldp_header, sizeof(cldp_header));
    
    // Send the packet
    if (sendto(sock, packet, ip_header.tot_len, 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr)) < 0) {
        perror("sendto failed");
    } else {
        char *query_type_str;
        switch (query_type) {
            case HOSTNAME_QUERY:
                query_type_str = "Hostname";
                break;
            case SYSTEM_TIME_QUERY:
                query_type_str = "System Time";
                break;
            case CPU_LOAD_QUERY:
                query_type_str = "CPU Load";
                break;
            default:
                query_type_str = "Unknown";
        }
        printf("QUERY message sent for %s (Transaction ID: %u)\n", 
              query_type_str, current_transaction_id);
        query_active = 1;
    }
}

// Process response message
void process_response(struct iphdr *ip_header, struct cldp_header *cldp_header, char *buffer) {
    // Extract transaction ID from response
    uint16_t response_transaction_id = ntohs(cldp_header->transaction_id);
    
    // Check if this response matches our query
    if (response_transaction_id == current_transaction_id) {
        // Extract payload length
        uint16_t payload_length = ntohs(cldp_header->payload_length);
        
        // Extract payload (metadata)
        char payload[MAX_PACKET_SIZE];
        memcpy(payload, buffer + sizeof(struct iphdr) + sizeof(struct cldp_header), payload_length);
        payload[payload_length] = '\0';  // Null-terminate the string
        
        // Print response information
        char *query_type_str;
        switch (cldp_header->query_type) {
            case HOSTNAME_QUERY:
                query_type_str = "Hostname";
                break;
            case SYSTEM_TIME_QUERY:
                query_type_str = "System Time";
                break;
            case CPU_LOAD_QUERY:
                query_type_str = "CPU Load";
                break;
            default:
                query_type_str = "Unknown";
        }
        
        printf("Response from %s:\n", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
        printf("  Query Type: %s\n", query_type_str);
        printf("  Data: %s\n", payload);
        printf("  Transaction ID: %u\n", response_transaction_id);
        
        // Mark query as complete
        query_active = 0;
    }
}

// Display menu
void display_menu() {
    printf("\nCLDP Client Menu:\n");
    printf("1. Send query for hostname\n");
    printf("2. Send query for system time\n");
    printf("3. Send query for CPU load\n");
    printf("4. Exit\n");
    printf("Enter your choice: ");
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr, dest_addr;
    char buffer[MAX_PACKET_SIZE];
    socklen_t addr_len = sizeof(server_addr);
    struct iphdr *ip_header;
    struct cldp_header *cldp_header;
    
    // Check if target IP address was provided
    if (argc != 2) {
        printf("Usage: %s <target_ip>\n", argv[0]);
        printf("Use broadcast address (255.255.255.255) to query all nodes\n");
        exit(EXIT_FAILURE);
    }
    
    // Set up signal handler for clean shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Create raw socket
    sock = socket(AF_INET, SOCK_RAW, CLDP_PROTOCOL);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket option to include IP header
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Initialize server address structure (for receiving)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Initialize destination address structure (for sending)
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    // Bind the socket (for receiving)
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("CLDP Client started. Target: %s\n", argv[1]);
    
    // Setup for periodic HELLO messages
    struct timeval last_hello_time, current_time;
    gettimeofday(&last_hello_time, NULL);
    
    int choice = 0;
    
    // Main loop
    while (running) {
        // Check if it's time to send a HELLO message (every 10 seconds)
        gettimeofday(&current_time, NULL);
        if (current_time.tv_sec - last_hello_time.tv_sec >= 10) {
            send_hello_message(sock, &dest_addr);
            last_hello_time = current_time;
        }
        
        // Show menu if no query is active
        if (!query_active) {
            display_menu();
            scanf("%d", &choice);
            
            switch (choice) {
                case 1:
                    send_query(sock, &dest_addr, HOSTNAME_QUERY);
                    break;
                case 2:
                    send_query(sock, &dest_addr, SYSTEM_TIME_QUERY);
                    break;
                case 3:
                    send_query(sock, &dest_addr, CPU_LOAD_QUERY);
                    break;
                case 4:
                    running = 0;
                    break;
                default:
                    printf("Invalid choice! Please try again.\n");
            }
        }
        
        // Set timeout for recvfrom
        struct timeval timeout;
        timeout.tv_sec = 1;  
        timeout.tv_usec = 0;
        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds); 
        
        // Wait for incoming packets or timeout
        int select_result = select(sock + 1, &read_fds, NULL, NULL, &timeout);
        
        if (select_result < 0) {
            if (running) {  // Only report error if not shutting down
                perror("select failed");
            }
            continue;
        } else if (select_result == 0) {
            // Timeout, continue to check for HELLO timing
            continue;
        }
        
        // Check if we have data to read from the socket
        if (FD_ISSET(sock, &read_fds)) {
            int packet_size = recvfrom(sock, buffer, MAX_PACKET_SIZE, 0, 
                                    (struct sockaddr *)&server_addr, &addr_len);
            
            if (packet_size < 0) {
                perror("recvfrom failed");
                continue;
            }
            
            // Parse IP header
            ip_header = (struct iphdr *)buffer;
            
            // Check if this is a CLDP packet
            if (ip_header->protocol == CLDP_PROTOCOL) {
                // Parse CLDP header
                cldp_header = (struct cldp_header *)(buffer + (ip_header->ihl * 4));
                
                // Process based on message type
                switch (cldp_header->msg_type) {
                    case HELLO_TYPE:
                        printf("Received HELLO from %s\n", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
                        break;
                    
                    case QUERY_TYPE:
                        // Client doesn't handle queries
                        break;
                    
                    case RESPONSE_TYPE:
                        if (query_active) {
                            process_response(ip_header, cldp_header, buffer);
                        }
                        break;
                    
                    default:
                        printf("Unknown message type: %d\n", cldp_header->msg_type);
                }
            }
        }
    }
    
    close(sock);
    printf("CLDP Client shut down gracefully.\n");
    return 0;
}