#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SERVER_TCP_PORT 8080   // TCP port to receive JSON data
#define SERVER_UDP_PORT 9876   // UDP port to receive UDP packets

void receive_json_over_tcp(int tcp_sockfd) {
    // Receive and process JSON data (code for JSON reception goes here)
    // For simplicity, let's assume JSON data is received as a string.
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

void receive_udp_packets(int udp_sockfd) {
    char udp_buffer[64]; // Buffer to receive UDP packets
    struct sockaddr_in udp_client_addr;
    socklen_t udp_addr_len = sizeof(udp_client_addr);

    printf("Server is listening on UDP port %d...\n", SERVER_UDP_PORT);

    while (1) {
        // Receive UDP packets
        ssize_t udpBytesReceived = recvfrom(udp_sockfd, udp_buffer, sizeof(udp_buffer), 0, (struct sockaddr *)&udp_client_addr, &udp_addr_len);
        if (udpBytesReceived < 0) {
            perror("UDP receive failed");
            continue; // Continue listening for other UDP packets
        }

        udp_buffer[udpBytesReceived] = '\0'; // Null-terminate the received data

        // Process the received UDP packet
        printf("Received UDP packet from %s:%d:\n%s\n", inet_ntoa(udp_client_addr.sin_addr), ntohs(udp_client_addr.sin_port), udp_buffer);
    }
}

int main() {
    int tcp_sockfd, udp_sockfd;
    struct sockaddr_in tcp_server_addr, udp_server_addr;
    socklen_t tcp_addr_len = sizeof(tcp_server_addr);
    socklen_t udp_addr_len = sizeof(udp_server_addr);

    // Create a TCP socket
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sockfd < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&tcp_server_addr, 0, sizeof(tcp_server_addr));

    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_port = htons(SERVER_TCP_PORT);
    tcp_server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the TCP socket to the server's address
    if (bind(tcp_sockfd, (struct sockaddr *)&tcp_server_addr, sizeof(tcp_server_addr)) == -1) {
        perror("TCP bind failed");
        close(tcp_sockfd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming TCP connections
    if (listen(tcp_sockfd, 5) == -1) {
        perror("TCP listen failed");
        close(tcp_sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on TCP port %d...\n", SERVER_TCP_PORT);

    // Accept a TCP connection
    int client_tcp_sockfd = accept(tcp_sockfd, (struct sockaddr *)&tcp_server_addr, &tcp_addr_len);
    if (client_tcp_sockfd < 0) {
        perror("TCP accept failed");
        close(tcp_sockfd);
        exit(EXIT_FAILURE);
    }

    // Create a UDP socket for receiving UDP packets
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0) {
        perror("UDP socket creation failed");
        close(client_tcp_sockfd);
        close(tcp_sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&udp_server_addr, 0, sizeof(udp_server_addr));

    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_port = htons(SERVER_UDP_PORT);
    udp_server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the UDP socket to the server's address
    if (bind(udp_sockfd, (struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr)) == -1) {
        perror("UDP bind failed");
        close(udp_sockfd);
        close(client_tcp_sockfd);
        close(tcp_sockfd);
        exit(EXIT_FAILURE);
    }

    // Call functions to receive JSON and UDP packets
    receive_json_over_tcp(client_tcp_sockfd);
    receive_udp_packets(udp_sockfd);

    // Close the UDP socket
    close(udp_sockfd);

    // Close the TCP sockets
    close(client_tcp_sockfd);
    close(tcp_sockfd);

    return 0;
}
