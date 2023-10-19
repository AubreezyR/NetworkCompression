#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include  <netdb.h>
#include <fcntl.h>
#include "cJSON.h"
//later this info comes from config file
#define SERVER_IP_ADDRESS ""
#define SOURCE_PORT_NUMBER_UDP 0
#define DESTINATION_PORT_NUMBER_UDP 0
#define DESTINATION_PORT_NUMBER_TCP_HEAD_SYN 0
#define DESTINATION_PORT_NUMBER_TCP_TAIL_SYN 0
#define PORT_NUMBER_TCP_PRE_PROBING_PHASES 0
#define PORT_NUMBER_TCP_POST_PROBING_PHASES 0
#define SIZE_OF_UDP_PAYLOAD_IN_BYTES 0
#define INTER_MEASUREMENT_TIME_IN_SECONDS 0 
#define NUMBER_OF_UDP_PACKETS_IN_PACKET_TRAIN 0 
#define TTL_FOR_UDP_PACKETS     
//TODO READ DATA FROM JSON INSTEAD OF HARDCODING IT

void asign_from_json(char* jsonFile){
	FILE *json_file = fopen(jsonFile, "r");
    if (json_file == NULL) {
        perror("Error opening JSON file");	
        exit(EXIT_FAILURE);
    }
    // Get the file size
    fseek(json_file, 0, SEEK_END);
    long file_size = ftell(json_file);
    fseek(json_file, 0, SEEK_SET);

    // Read the JSON data into a buffer
    char *json_buffer = (char *)malloc(file_size + 1);
    if (!json_buffer) {
        perror("Memory allocation error");
        fclose(json_file);
        exit(1);
    }
    fread(json_buffer, 1, file_size, json_file);
    json_buffer[file_size] = '\0'; // Null-terminate the string

    fclose(json_file);

    // Parse the JSON data
    cJSON *json = cJSON_Parse(json_buffer);
    if (!json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        free(json_buffer);
        exit(1);
    }

 // Extract and assign values from the parsed JSON
   cJSON *config = cJSON_GetObjectItem(json, "config");
   if (config !=  NULL) {
       cJSON *item;

       item = cJSON_GetObjectItem(config, "ServerIPAddress");
       if (item && item->type == cJSON_String) {
           snprintf(SERVER_IP_ADDRESS, sizeof(SERVER_IP_ADDRESS), "%s", item->valuestring);
       }

       item = cJSON_GetObjectItem(config, "SourcePortNumberUDP");
       if(item && item->type == cJSON_Number) {
           SOURCE_PORT_NUMBER_UDP  item->valueint;
       }

       item = cJSON_GetObjectItem(config, "DestinationPortNumberUDP");
       if (item && item->type == cJSON_Number) {
           DESTINATION_PORT_NUMBER_UDP  item->valueint;
       }

       item = cJSON_GetObjectItem(config, "DestinationPortNumberTCPHeadSYN");
       if (item && item->type == cJSON_Number) {
           DESTINATION_PORT_NUMBER_TCP_HEAD_SYN item->valueint;
       }

       item = cJSON_GetObjectItem(config, "DestinationPortNumberTCPTailSYN");
       if (item && item->type == cJSON_Number) {
           DESTINATION_PORT_NUMBER_TCP_TAIL_SYN  item->valueint;
       }

       item = cJSON_GetObjectItem(config, "PortNumberTCP_PreProbingPhases");
       if (item && item->type == cJSON_Number) {
           PORT_NUMBER_TCP_PRE_PROBING_PHASES  item->valueint;
       }

       item = cJSON_GetObjectItem(config, "PortNumberTCP_PostProbingPhases");
       if (item && item->type == cJSON_Number) {
           PORT_NUMBER_TCP_POST_PROBING_PHASES  item->valueint;
       }

       item = cJSON_GetObjectItem(config, "SizeOfUDPPayloadInBytes");
       if (item && item->type == cJSON_Number) {
           SIZE_OF_UDP_PAYLOAD_IN_BYTES  item->valueint;
       }

       item = cJSON_GetObjectItem(config, "InterMeasurementTimeInSeconds");
       if (item && item->type == cJSON_Number) {
           INTER_MEASUREMENT_TIME_IN_SECONDS  item->valueint;
       }

       item = cJSON_GetObjectItem(config, "NumberOfUDPPacketsInPacketTrain");
       if (item && item->type == cJSON_Number) {
           NUMBER_OF_UDP_PACKETS_IN_PACKET_TRAIN  item->valueint;
       }

       item = cJSON_GetObjectItem(config, "TTLForUDPPackets");
       if (item && item->type == cJSON_Number) {
           TTL_FOR_UDP_PACKETS  item->valueint;
       }
   }

    cJSON_Delete(json);
    free(json_buffer);


    
}

void send_json_over_tcp(char* jsonFile) {
    int s;
   	int status;
   	struct addrinfo hints;
   	struct addrinfo *servinfo;
   	
   	   	
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // ipv4 or v6 AF_INET is v4 AF_INET6 is v6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;//asigns locall host ip address to socket

    if((status = getaddrinfo(SERVER_IP_ADDRESS, PORT_NUMBER_TCP_PRE_PROBING_PHASES, &hints, &servinfo)) != 0){//replace NULL with an actuall website or IP if you want
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));    
		exit(1);
    }
   	//set up socket
   	s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

   	if(connect(s,servinfo->ai_addr, servinfo->ai_addrlen) < 0){
   		perror("Connect error");
   		close(s);
   	}
	// Read and send the JSON file over TCP
    FILE *json_file = fopen(jsonFile, "r");
    if (json_file == NULL) {
        perror("Error opening JSON file");
        close(s);	
        exit(EXIT_FAILURE);
    }
    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), json_file)) > 0) {
    	send(s, buffer, bytesRead, 0);
    }
    fclose(json_file);
   	close(s);   	
   	
}

void send_udp_packets(int packet_type) {
    int s;
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or v6
    hints.ai_socktype = SOCK_DGRAM; // Use UDP
    hints.ai_flags = AI_PASSIVE; // Assign local host IP address to socket

    if ((status = getaddrinfo("192.168.128.3", "8765", &hints, &servinfo) != 0)) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // Set up socket
    s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    // Create the packet
    char packet_low[100];
    char packet_high[100];

    // Fill the string with null characters
    memset(packet_low, '0', sizeof(packet_low));

    // Open /dev/urandom as a source of randomness if we are doing a high packet
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd == -1) {
        perror("open /dev/urandom");
        exit(1);
    }

    // Read random data from /dev/urandom to replace null characters
    if (read(urandom_fd, packet_high, sizeof(packet_high)) == -1) {
        perror("read /dev/urandom");
        close(urandom_fd);
        exit(1);
    }

    close(urandom_fd);

    // Send the data to the server
    if (sendto(s, packet_low, sizeof(packet_low), 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("sendto");
        exit(1);
    }
    sleep(INTER_MEASUREMENT_TIME_IN_SECONDS);

    // Send the data to the server
    if (sendto(s, packet_high, sizeof(packet_high), 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);
    close(s);
}



int main(int argc, char *argv[]) {
	//error check
	if(argc != 2){
		printf("Error: Incorrect number of arguments");
		return EXIT_FAILURE;
	}
	//TCP and JSON
	printf("Sending JSON...");
	send_json_over_tcp(argv[1]);
	printf("JSON sent, Sending low packets...");
	send_udp_packets(0);
	printf("low packets sent, waiting 15 secs....");
	
	// recieve from server if there was compression
    
	
    return 0;
}
