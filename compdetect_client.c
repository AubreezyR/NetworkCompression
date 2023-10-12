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
#define SERVER_TCP_PORT 12345
#define SOURCE_UDP_PORT 9876
#define DEST_UDP_PORT 8765
#define DEST_TCP_HEAD_SYN_PORT 9999
#define DEST_TCP_TAIL_SYN_PORT 8888
#define PORT_TCP_PRE_PROBING 7777
#define PORT_TCP_POST_PROBING 6666
#define UDP_PAYLOAD_SIZE 1000
#define INTER_MEASUREMENT_TIME 15
#define NUM_UDP_PACKETS 6000
#define UDP_TTL 255

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

void receive_data() {
    // Create a UDP socket to receive data
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("UDP socket creation failed");
        return;
    }

    // Bind the UDP socket to the source port
    struct sockaddr_in client_udp_addr;
    memset(&client_udp_addr, 0, sizeof(client_udp_addr));
    client_udp_addr.sin_family = AF_INET;
    client_udp_addr.sin_port = htons(SOURCE_UDP_PORT);
    client_udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(udp_socket, (struct sockaddr *)&client_udp_addr, sizeof(client_udp_addr)) < 0) {
        perror("UDP bind failed");
        close(udp_socket);
        return;
    }

    // Define the server's UDP address
    struct sockaddr_in server_udp_addr;
    memset(&server_udp_addr, 0, sizeof(server_udp_addr));
    server_udp_addr.sin_family = AF_INET;
    server_udp_addr.sin_port = htons(DEST_UDP_PORT);
    server_udp_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Receive data from the server
    char status;
    int bytes_received = recvfrom(udp_socket, &status, sizeof(status), 0, NULL, NULL);

    if (bytes_received == sizeof(status)) {
        if (status == '0') {
            printf("Data wasn't compressed (status: %c)\n", status);
        } else if (status == '1') {
            printf("Data was compressed (status: %c)\n", status);
        } else {
            printf("Received unknown status: %c\n", status);
        }
    } else {
        perror("Error while receiving status");
    }

    // Close the UDP socket
    close(udp_socket);
}

int main(int argc, char* argv[]) {
	if(argc != 2){
		perror("Wrong number of arguments");
	}
    // Send JSON data to the server
    send_json_data(argv[1]);

    // Receive and process data from the server
    receive_data();

    return 0;
}
