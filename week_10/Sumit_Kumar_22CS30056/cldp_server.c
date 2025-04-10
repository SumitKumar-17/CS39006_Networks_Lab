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
#include <sys/sysinfo.h>
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

void handle_signal(int signo) {
    running = 0;
    printf("\nShutting down the CLDP server...\n");
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

void get_hostname(char *buffer, int *length) {
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    *length = snprintf(buffer, 256, "%s", hostname);
}

void get_system_time(char *buffer, int *length) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    time_t now_time = tv.tv_sec;
    struct tm *now_tm = localtime(&now_time);
    
    *length = strftime(buffer, 256, "%Y-%m-%d %H:%M:%S", now_tm);
}

void get_cpu_load(char *buffer, int *length) {
    struct sysinfo info;
    sysinfo(&info);
    
    float load_1 = info.loads[0] / 65536.0;
    float load_5 = info.loads[1] / 65536.0;
    float load_15 = info.loads[2] / 65536.0;
    
    *length = snprintf(buffer, 256, "Load avg: %.2f, %.2f, %.2f", load_1, load_5, load_15);
}

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
    
    // Set broadcast address
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

// Process query and send response
void process_query_and_respond(int sock, struct iphdr *ip_header, struct cldp_header *cldp_header, struct sockaddr_in *src_addr) {
    // Prepare response packet
    char response_packet[MAX_PACKET_SIZE];
    struct iphdr response_ip;
    struct cldp_header response_cldp;
    char payload[256];
    int payload_length = 0;
    
    // Generate response based on query type
    switch (cldp_header->query_type) {
        case HOSTNAME_QUERY:
            get_hostname(payload, &payload_length);
            printf("Query for hostname received, sending: %s\n", payload);
            break;
        case SYSTEM_TIME_QUERY:
            get_system_time(payload, &payload_length);
            printf("Query for system time received, sending: %s\n", payload);
            break;
        case CPU_LOAD_QUERY:
            get_cpu_load(payload, &payload_length);
            printf("Query for CPU load received, sending: %s\n", payload);
            break;
        default:
            printf("Unknown query type: %d\n", cldp_header->query_type);
            return;
    }
    
    // Set IP header
    memset(&response_ip, 0, sizeof(response_ip));
    response_ip.ihl = 5;
    response_ip.version = 4;
    response_ip.tos = 0;
    response_ip.tot_len = sizeof(struct iphdr) + sizeof(struct cldp_header) + payload_length;
    response_ip.id = htons(rand() % 65535);
    response_ip.frag_off = 0;
    response_ip.ttl = 64;
    response_ip.protocol = CLDP_PROTOCOL;
    response_ip.check = 0;
    
    // Swap source and destination addresses for response
    response_ip.saddr = ip_header->daddr;
    response_ip.daddr = ip_header->saddr;
    
    // Calculate IP header checksum
    response_ip.check = calculate_checksum((unsigned short *)&response_ip, sizeof(struct iphdr));
    
    // Set CLDP header for response
    memset(&response_cldp, 0, sizeof(response_cldp));
    response_cldp.msg_type = RESPONSE_TYPE;
    response_cldp.query_type = cldp_header->query_type;
    response_cldp.payload_length = htons(payload_length);
    response_cldp.transaction_id = cldp_header->transaction_id;  // Match the transaction ID
    response_cldp.reserved = 0;
    
    // Assemble the response packet
    memcpy(response_packet, &response_ip, sizeof(response_ip));
    memcpy(response_packet + sizeof(response_ip), &response_cldp, sizeof(response_cldp));
    memcpy(response_packet + sizeof(response_ip) + sizeof(response_cldp), payload, payload_length);
    
    // Update destination address for sending
    src_addr->sin_addr.s_addr = ip_header->saddr;
    
    if (sendto(sock, response_packet, response_ip.tot_len, 0, (struct sockaddr *)src_addr, sizeof(*src_addr)) < 0) {
        perror("sendto failed for response");
    } else {
        printf("Response sent for query type %d\n", cldp_header->query_type);
    }
}

int main() {
    int sock;
    struct sockaddr_in server_addr, client_addr, broadcast_addr;
    char buffer[MAX_PACKET_SIZE];
    socklen_t client_addr_len = sizeof(client_addr);
    struct iphdr *ip_header;
    struct cldp_header *cldp_header;
    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
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

    // Enable broadcast
    int broadcast_enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt SO_BROADCAST failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Initialize broadcast address structure
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("CLDP Server started. Listening for packets...\n");
    
    struct timeval last_hello_time, current_time;
    gettimeofday(&last_hello_time, NULL);
    
    while (running) {
        // Check if it's time to send a HELLO message (every 10 seconds)
        gettimeofday(&current_time, NULL);
        if (current_time.tv_sec - last_hello_time.tv_sec >= 10) {
            send_hello_message(sock, &broadcast_addr);
            last_hello_time = current_time;
        }
        
        // Set timeout for recvfrom to check for HELLO timing
        struct timeval timeout;
        timeout.tv_sec = 1; 
        timeout.tv_usec = 0;
        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        
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
        
        // We have data to read
        int packet_size = recvfrom(sock, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        
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
            
            // Print info about the received packet
            printf("Received CLDP packet from %s, Type: ", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
            
            // Process based on message type
            switch (cldp_header->msg_type) {
                case HELLO_TYPE:
                    printf("HELLO\n");
                    // Just log for HELLO messages
                    break;
                
                case QUERY_TYPE:
                    printf("QUERY (Type: %d)\n", cldp_header->query_type);
                    // Process query and send response
                    process_query_and_respond(sock, ip_header, cldp_header, &client_addr);
                    break;
                
                case RESPONSE_TYPE:
                    printf("RESPONSE (Type: %d)\n", cldp_header->query_type);
                    // Server doesn't process responses
                    break;
                
                default:
                    printf("Unknown message type: %d\n", cldp_header->msg_type);
            }
        }
    }
    
    close(sock);
    printf("CLDP Server shut down gracefully.\n");
    return 0;
}
