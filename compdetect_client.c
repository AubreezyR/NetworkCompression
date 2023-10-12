#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "cJSON.h"

#define SERVER_IP "SERVER_IP_ADDRESS"
#define SERVER_TCP_PORT 12345
#define SERVER_UDP_PORT 8765
#define LOW_ENTROPY_PACKET_COUNT 10
#define HIGH_ENTROPY_PACKET_COUNT 10
#define INTER_MEASUREMENT_TIME 5 // Inter-measurement time in seconds
#define THRESHOLD 100 // Threshold for compression detection (in milliseconds)

void send_json_data() {
    // Create a TCP socket to connect to the server
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        perror("TCP socket creation failed");
        return;
    }

    // Define the server's TCP address
    struct sockaddr_in server_tcp_addr;
    memset(&server_tcp_addr, 0, sizeof(server_tcp_addr));
    server_tcp_addr.sin_family = AF_INET;
    server_tcp_addr.sin_port = htons(SERVER_TCP_PORT);
    server_tcp_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to the server over TCP
    if (connect(tcp_socket, (struct sockaddr *)&server_tcp_addr, sizeof(server_tcp_addr)) < 0) {
        perror("TCP connection failed");
        close(tcp_socket);
        return;
    }

    // Create and send JSON data
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", "John");
    cJSON_AddNumberToObject(root, "age", 30);

    char *json_data = cJSON_Print(root);
    send(tcp_socket, json_data, strlen(json_data), 0);

    cJSON_Delete(root);
    free(json_data);

    // Close the TCP socket
    close(tcp_socket);
}

void send_udp_packets(int entropy) {
    // Create a UDP socket
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("UDP socket creation failed");
        return;
    }

    // Define the server's UDP address
    struct sockaddr_in server_udp_addr;
    memset(&server_udp_addr, 0, sizeof(server_udp_addr));
    server_udp_addr.sin_family = AF_INET;
    server_udp_addr.sin_port = htons(SERVER_UDP_PORT);
    server_udp_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Generate and send UDP packets
    for (int i = 0; i < entropy; i++) {
        // Create and send UDP packets
        char packet_data[128];
        snprintf(packet_data, sizeof(packet_data), "UDP Packet %d (Entropy %d)", i, entropy);
        sendto(udp_socket, packet_data, strlen(packet_data), 0, (struct sockaddr *)&server_udp_addr, sizeof(server_udp_addr));
    }

    // Close the UDP socket
    close(udp_socket);
}

int main() {
    // Send JSON data to the server
    send_json_data();

    // Sleep for inter-measurement time (gamma)
    sleep(INTER_MEASUREMENT_TIME);

    // Send UDP packets with low entropy data
    send_udp_packets(LOW_ENTROPY_PACKET_COUNT);

    // Sleep for inter-measurement time (gamma)
    sleep(INTER_MEASUREMENT_TIME);

    // Send UDP packets with high entropy data
    send_udp_packets(HIGH_ENTROPY_PACKET_COUNT);

    // Receive compression detection result from the server
    int compression_detected;
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_tcp_addr;
    // Fill in server_tcp_addr with the server's information (same as in send_json_data)

    if (connect(tcp_socket, (struct sockaddr *)&server_tcp_addr, sizeof(server_tcp_addr)) < 0) {
        perror("TCP connection failed");
        close(tcp_socket);
        return 1;
    }

    recv(tcp_socket, &compression_detected, sizeof(compression_detected), 0);
    close(tcp_socket);

    // Print an appropriate message based on the result
    if (compression_detected) {
        printf("Compression detected!\n");
    } else {
        printf("No compression was detected.\n");
    }

    return 0;
}
