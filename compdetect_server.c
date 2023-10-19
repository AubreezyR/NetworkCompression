#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include "cJSON.h"

#define SERVER_TCP_PORT 7777   // TCP port to receive JSON data
#define SERVER_UDP_PORT "8765"   // UDP port to receive UDP packets
#define PACKET_SIZE 1400     
#define PACKET_COUNT 10      
#define THRESHOLD 100

cJSON*json_dict;
//TODO READ DATA FROM JSON INSTEAD OF HARDCODING IT

void asign_from_json(char* jsonFile) {

	FILE *file = fopen(jsonFile, "r");
    if (!file) {
        perror("Failed to open the JSON file");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *json_data = (char *)malloc(file_size + 1);
    if (!json_data) {
        fclose(file);
        perror("Memory allocation failed");
        exit(1);
    }

    fread(json_data, 1, file_size, file);
    fclose(file);
    json_data[file_size] = '\0';

    // Parse the JSON data
    json_dict = cJSON_Parse(json_data);
    free(json_data);

    if (json_dict == NULL) {
        perror("Failed to parse JSON data");
        exit(1);
    }
    // Access values from the JSON object and store them as variables
    /*
    SERVER_IP_ADDRESS = ;
    SOURCE_PORT_NUMBER_UDP = ;
    DESTINATION_PORT_NUMBER_UDP = cJSON_GetObjectItem(root, "DestinationPortNumberUDP")->valuestring;
    DESTINATION_PORT_NUMBER_TCP_HEAD_SYN = cJSON_GetObjectItem(root, "DestinationPortNumberTCPHeadSYN")->valuestring;
    DESTINATION_PORT_NUMBER_TCP_TAIL_SYN = cJSON_GetObjectItem(root, "DestinationPortNumberTCPTailSYN")->valuestring;
    PORT_NUMBER_TCP_PRE_PROBING_PHASES = cJSON_GetObjectItem(root, "PortNumberTCP_PreProbingPhases")->valuestring;
    PORT_NUMBER_TCP_POST_PROBING_PHASES = cJSON_GetObjectItem(root, "PortNumberTCP_PostProbingPhases")->valuestring;
    SIZE_OF_UDP_PAYLOAD_IN_BYTES = cJSON_GetObjectItem(root, "SizeOfUDPPayloadInBytes")->valuestring;
    INTER_MEASUREMENT_TIME_IN_SECONDS = cJSON_GetObjectItem(root, "InterMeasurementTimeInSeconds")->valuestring;
    NUMBER_OF_UDP_PACKETS_IN_PACKET_TRAIN = cJSON_GetObjectItem(root, "NumberOfUDPPacketsInPacketTrain")->valuestring;
    TTL_FOR_UDP_PACKETS = cJSON_GetObjectItem(root, "TTLForUDPPackets")->valuestring;*/   
    
}

void receive_json_over_tcp() {
	int s, new_s;
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;
    char json_buffer[1042];
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    char* ip = cJSON_GetObjectItem(json_dict, "ServerIPAddress")->valuestring;
    char*  port = cJSON_GetObjectItem(json_dict, "PortNumberTCP_PreP")->valuestring;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // ipv4 or v6 AF_INET is v4 AF_INET6 is v6
    hints.ai_socktype = SOCK_STREAM;

    if((status = getaddrinfo(ip, port, &hints, &servinfo)) != 0){//replace NULL with an actuall website or IP if you want
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
  	char*  jsonString = cJSON_Print(root);
   	printf("JSON DATA:\n%s\n", jsonString);

	freeaddrinfo(servinfo);
	close(s);
 }
    


void receive_udp_packets() {
    int sockfd;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    char buffer[1042];

    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // Use UDP

    if (getaddrinfo("192.168.128.3", SERVER_UDP_PORT, &hints, &res) != 0) {
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

    freeaddrinfo(res);

    printf("Waiting for UDP packets on port %s...\n", SERVER_UDP_PORT);

    while (1) {
        addr_size = sizeof their_addr;
        ssize_t num_bytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&their_addr, &addr_size);
        if (num_bytes < 0) {
            perror("recvfrom");
            exit(1);
        }

        buffer[num_bytes] = '\0'; // Null-terminate the received data
        printf("Received: %s\n", buffer);
    }
	freeaddrinfo(res);
    close(sockfd);
}

int main() {
    // Call functions to receive JSON and UDP packets
    printf("starting connection...");
    receive_json_over_tcp();
    receive_udp_packets();
    return 0;
}
