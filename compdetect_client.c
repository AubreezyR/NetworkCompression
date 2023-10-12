#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "cJSON.h"

#define SERVER_IP "192.168.128.3"
#define SERVER_TCP_PORT 8080
#define SERVER_UDP_PORT 8765
#define LOW_ENTROPY_PACKET_COUNT 10
#define HIGH_ENTROPY_PACKET_COUNT 10
#define INTER_MEASUREMENT_TIME 5 // Inter-measurement time in seconds
#define THRESHOLD 100 // Threshold for compression detection (in milliseconds)

void send_json_data(char* jsonFile) {
    // Open the JSON file for reading
    FILE *config_file = fopen(jsonFile, "r");
    if (!config_file) {
        perror("Failed to open config.json");
        return;
    }

    // Calculate the file size
    fseek(config_file, 0, SEEK_END);
    long file_size = ftell(config_file);
    fseek(config_file, 0, SEEK_SET);

    // Read the file content into a buffer
    char *json_data = (char *)malloc(file_size + 1);
    fread(json_data, 1, file_size, config_file);
    json_data[file_size] = '\0';

    // Close the file
    fclose(config_file);

    // Create a TCP socket to connect to the server
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        perror("TCP socket creation failed");
        free(json_data);
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
        free(json_data);
        return;
    }

    // Send the JSON data to the server
    send(tcp_socket, json_data, strlen(json_data), 0);

    // Close the TCP socket and free allocated memory
    close(tcp_socket);
    free(json_data);
}

void send_udp_packets(int entropy) {
	printf("sending udp");
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

int main(int argc, char*argv[]) {
	if(argc != 2){
		perror("Wrong number of args");
	}
    // Send JSON data to the server
    printf("sending json");
    send_json_data(argv[1]);

    // Sleep for inter-measurement time
    printf("sleeping");
    sleep(INTER_MEASUREMENT_TIME);

    // Send UDP packets with low entropy data
    printf("sending low entropy");
    send_udp_packets(LOW_ENTROPY_PACKET_COUNT);

    // Sleep for inter-measurement time
    printf("sleeping");
    sleep(INTER_MEASUREMENT_TIME);

    // Send UDP packets with high entropy data
    printf("sending high entropy");
    send_udp_packets(HIGH_ENTROPY_PACKET_COUNT);

    // Receive compression detection result from the server
    printf("setting up tcp to recieve results");
    int compression_detected;
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_tcp_addr;
	memset(&server_tcp_addr, 0, sizeof(server_tcp_addr));
    server_tcp_addr.sin_family = AF_INET;
    server_tcp_addr.sin_port = htons(SERVER_TCP_PORT);
    server_tcp_addr.sin_addr.s_addr =inet_addr(SERVER_IP);

    printf("connecting to server");
    if (connect(tcp_socket, (struct sockaddr *)&server_tcp_addr, sizeof(server_tcp_addr)) < 0) {
        perror("TCP connection failed");
        close(tcp_socket);
        return 1;
    }

	printf("reciving");
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
