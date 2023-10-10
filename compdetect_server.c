#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SERVER_TCP_PORT 8080   // TCP port to receive JSON data
#define SERVER_UDP_PORT 8765   // UDP port to receive UDP packets
#define PACKET_SIZE 1400     
#define PACKET_COUNT 10      
#define THRESHOLD 100

void receive_json_over_tcp(int tcp_sockfd) {
    // Receive and process JSON data (code for JSON reception goes here)
    char json_buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = recv(tcp_sockfd, json_buffer, sizeof(json_buffer), 0)) > 0) {
        // Process JSON data (json_buffer)
        // Write the received JSON data to a file
        FILE *json_file = fopen("received_data.json", "w");
        if (json_file == NULL) {
            perror("Error opening file for writing");
         
        }
    
        size_t bytesWritten = fwrite(json_buffer, 1, bytesRead, json_file);
        fclose(json_file);
    
        printf("Received JSON data and saved it to received_data.json\n");
    }
}

void receive_udp_packets() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char packet[PACKET_SIZE];
    socklen_t client_addr_len = sizeof(client_addr);

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_UDP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified UDP port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("UDP socket binding failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Receive and process UDP packets
    for (int i = 0; i < PACKET_COUNT * 2; i++) {
        ssize_t bytes_received = recvfrom(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &client_addr_len);

        if (bytes_received < 0) {
            printf("UDP packet reception error");
            continue; // Skip processing on error
        }

        printf("Received UDP packet %d", i);
    }

    close(sockfd);
}

int main() {
    // Call functions to receive JSON and UDP packets
    //receive_json_over_tcp(client_tcp_sockfd);
    receive_udp_packets();
    return 0;
}
