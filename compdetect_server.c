#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <time.h>
#include "cJSON.h"

#define SERVER_TCP_PORT 7777   // TCP port to receive JSON data
#define SERVER_UDP_PORT "8765"   // UDP port to receive UDP packets
#define PACKET_SIZE 1400     
#define PACKET_COUNT 10      
#define THRESHOLD 100

cJSON*json_dict;

cJSON* receive_json_over_tcp(char* port) {
	int s, new_s;
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;
    char json_buffer[1042];
    struct sockaddr_storage their_addr;
    socklen_t addr_size;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // ipv4 or v6 AF_INET is v4 AF_INET6 is v6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0){//replace NULL with an actuall website or IP if you want
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));    
		exit(1);
    }
	//set up socket
	s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	//bind socket to port
	if(bind(s, servinfo->ai_addr, servinfo->ai_addrlen)< 0){
		perror("TCP socket bind fail");
		close(s);
		exit(1);
	}
	//listen for connection
	if(listen(s, 10) < 0){
		perror("listen error");
		close(s);
		exit(1);
	}
	
	addr_size = sizeof(their_addr);
	new_s = accept(s, (struct sockaddr *)&their_addr, &addr_size);

	if(new_s < 0){
		perror("Accept error");
		close(s);
		exit(1);
	}
	if(recv(new_s, json_buffer, sizeof(json_buffer), 0)< 0){
		perror("recv error");
		close(s);
		exit(1);
	}

	// convert the json buffer to a dictionary
   	json_buffer[sizeof(json_buffer) - 1] = '\0';
   	cJSON* root = cJSON_Parse(json_buffer);
  	return root;

	freeaddrinfo(servinfo);
	close(s);
 }
    


void receive_udp_packets() {
    int sockfd;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    char buffer[1042];
    struct addrinfo hints, *res, *p;
    char* ip = cJSON_GetObjectItem(json_dict, "ServerIPAddress")->valuestring;
    int portInt = cJSON_GetObjectItem(json_dict, "DestinationPortNumberUDP")->valueint;
   	char port[5];
   	sprintf(port, "%d", portInt); 
	
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // Use UDP

    if (getaddrinfo(ip, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
       exit(1);
    }

// Iterate through the list of results to find a suitable address to bind
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }

        break;  // Successfully bound the socket
    }

    printf("Waiting for UDP packets on port %s...\n", ip);
	int i = 0;
	clock_t start_time, end_time;
	double elapsed_time;
	start_time = clock();
    while (i < (cJSON_GetObjectItem(json_dict, "NumberOfUDPPacketsInPacketTrain")->valueint) *2) {
        addr_size = sizeof their_addr;
        ssize_t num_bytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&their_addr, &addr_size);
        if (num_bytes < 0) {
            perror("recvfrom");
            exit(1);
        }

        buffer[num_bytes] = '\0'; // Null-terminate the received data
        //printf("Received: %s\n", buffer);
        i++;
    }
    end_time = clock();
    elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("total time for packet train: %f\n", elapsed_time);
	freeaddrinfo(res);
    close(sockfd);
}

int main(int argc, char* argv[]) {
	if(argc != 2){
		perror("Error: Wrong number of args");
	}
    // Call functions to receive JSON and UDP packets
    printf("starting connection...");
    json_dict = receive_json_over_tcp(argv[1]);
    receive_udp_packets();
    return 0;
}
