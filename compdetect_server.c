#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "cJSON.h"

#define SERVER_TCP_PORT 8080   // TCP port to receive JSON data
#define SERVER_UDP_PORT 8765   // UDP port to receive UDP packets
#define PACKET_SIZE 1400     
#define PACKET_COUNT 10      
#define THRESHOLD 100

void receive_json_over_tcp() {
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
    server_tcp_addr.sin_port = htons(SERVER_TCP_PORT); //converts port into network btye order
   
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
   	json_buffer[json_bytes_recieved] = '\0';
   	cJSON* root = cJSON_Parse(json_buffer);
   	print(root);
    	
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
    receive_json_over_tcp();
    receive_udp_packets();
    return 0;
}
