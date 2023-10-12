#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include "cJSON.h"

#define SERVER_TCP_PORT 8080   // TCP port to receive JSON data
#define SERVER_UDP_PORT 8765   // UDP port to receive UDP packets
#define CLIENT_TCP_PORT 8080
#define PACKET_SIZE 1400     
#define PACKET_COUNT 10      
#define THRESHOLD 100

void receive_json_over_tcp(unsigned short  server_port) {

	char json_buffer[1042];
    // Create the TCP socket MAKE SURE TO ERROR CHECK
	int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_socket < 0) {
        perror("TCP socket creation failed");
        return;
	}
    // Define TCP struct members
    struct sockaddr_in server_tcp_addr;
    memset(&server_tcp_addr, 0, sizeof(server_tcp_addr));
    server_tcp_addr.sin_family = AF_INET;
    server_tcp_addr.sin_port = htons(server_port); //converts port into network btye order

    // Bind the socket
	if(bind(tcp_socket,(struct sockaddr *)&server_tcp_addr, sizeof(server_tcp_addr)) < 0){
		printf("TCP socket binding failed");
		close(tcp_socket);
		return;
	}
    // Listen for incoming connections
	if(listen(tcp_socket,1)< 0){
		printf("TCP socket listening failed");
		close(tcp_socket);
		return;
	}
    // Accept the connection
	int client_tcp_socket = accept(tcp_socket, NULL, NULL);
	if(client_tcp_socket < 0){
		printf("TCP socket accept failed");
		close(tcp_socket);
		return;
	}
    // get JSON data 
	ssize_t json_bytes_recieved = recv(client_tcp_socket, json_buffer, sizeof(json_buffer), 0);
	if(json_bytes_recieved < 0){
		printf("Error receving JSON data");
		close(client_tcp_socket);
		close(tcp_socket);
		return;
	}	
    // convert the json buffer to a dictionary
	//json_buffer[json_bytes_recieved] = '\0';
	//cJSON* root = cJSON_Parse(json_buffer);
    //create variables based on the dictionary
    //char *json_string = cJSON_Print(root);
	
}

void receive_udp_packets(int* compression_detected, unsigned short server_port) {
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
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified UDP port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("UDP socket binding failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

	//calc start time 
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	
    // Receive and process UDP packets
    for (int i = 0; i < PACKET_COUNT * 2; i++) {
        ssize_t bytes_received = recvfrom(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &client_addr_len);

        if (bytes_received < 0) {
            printf("UDP packet reception error");
            continue; // Skip processing on error
        }

        printf("Received UDP packet %d", i);
    }
	//calc end time
	clock_gettime(CLOCK_MONOTONIC, &end);

	//calc time diff 
	long delta_time = (end.tv_sec - start.tv_sec)* 1000; //convert to milli seconds for readablility use .tv_sec so we can preform subtraction
	printf("delta time is: %ld\n", delta_time);
	// Simulate compression detection logic (you can customize this logic)
    if (delta_time > THRESHOLD) {
        *compression_detected = 1; // Compression detected
        printf("compression dectected");
    } else {
        *compression_detected = 0; // No compression detected
        printf("compression not  dectected");
    }
	
    close(sockfd);
}

int send_compression_results(int* compression_detected){
	// Establish a new TCP connection for sending findings to the client
    int new_tcp_socket;
    struct sockaddr_in client_tcp_addr_new;

    // Create a new TCP socket for communicating findings to the client
    new_tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (new_tcp_socket < 0) {
        perror("TCP socket creation failed");
        return 1;
    }

    memset(&client_tcp_addr_new, 0, sizeof(client_tcp_addr_new));
    client_tcp_addr_new.sin_family = AF_INET;
    client_tcp_addr_new.sin_port = htons(CLIENT_TCP_PORT);
    client_tcp_addr_new.sin_addr.s_addr = INADDR_ANY;

    // Connect to the client over the new TCP connection
    if (connect(new_tcp_socket, (struct sockaddr *)&client_tcp_addr_new, sizeof(client_tcp_addr_new) < 0)) {
        perror("TCP connection failed");
        close(new_tcp_socket);
        return 1;
    }

    // Send the compression detection result to the client over the new TCP connection
    if (send(new_tcp_socket, &compression_detected, sizeof(compression_detected), 0) < 0) {
        perror("Sending compression detection result failed");
    }

    close(new_tcp_socket);

    return 0;
}

int main(int argc, char *argv[]) {
	//error check for agrs
	if(argc != 2 ){
		printf("Error: Wrong number of args");
		return -1;
	}
	//convert argv[1] which is the port number to an unsigned short
	unsigned short server_port = (unsigned short)atoi(argv[1]);
    // Call functions to receive JSON and UDP packets
    printf("reciving json");
    receive_json_over_tcp(server_port);
    printf("json recived");
    int* compression_detected = 0;
    printf("getting udp packers");
    receive_udp_packets(compression_detected, server_port);
    printf("udp packets got");
    //compression_detected is now either 1 or 0 now we send this to the client
    printf("sending results");
    send_compression_results(compression_detected);
    printf("results send");

    return 0;
}
