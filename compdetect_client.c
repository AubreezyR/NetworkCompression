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
char* SERVER_IP_ADDRESS;
char* SOURCE_PORT_NUMBER_UDP;
char* DESTINATION_PORT_NUMBER_UDP;
char* DESTINATION_PORT_NUMBER_TCP_HEAD_SYN;
char* DESTINATION_PORT_NUMBER_TCP_TAIL_SYN;
char* PORT_NUMBER_TCP_PRE_PROBING_PHASES;
char* PORT_NUMBER_TCP_POST_PROBING_PHASES;
char* SIZE_OF_UDP_PAYLOAD_IN_BYTES;
char* INTER_MEASUREMENT_TIME_IN_SECONDS;
char* NUMBER_OF_UDP_PACKETS_IN_PACKET_TRAIN;
char* TTL_FOR_UDP_PACKETS;
//TODO READ DATA FROM JSON INSTEAD OF HARDCODING IT

void asign_from_json(char* jsonFile) {
    // Read the JSON data from the file
    FILE* file = fopen(jsonFile, "r");
    if (file == NULL) {
        perror("Error opening JSON file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* json_data = (char*)malloc(file_size + 1);
    fread(json_data, 1, file_size, file);
    fclose(file);
    json_data[file_size] = '\0';

    // Parse the JSON data
    cJSON* root = cJSON_Parse(json_data);
    if (root == NULL) {
        fprintf(stderr, "Error parsing JSON data: %s\n", cJSON_GetErrorPtr());
        free(json_data);
        exit(EXIT_FAILURE);
    }

    // Iterate through the JSON object's key-value pairs and store as char*
    cJSON* current_item = root->child;
    while (current_item != NULL) {
        const char* key = current_item->string;
        cJSON* value = current_item;

        if (strcmp(key, "ServerIPAddress") == 0) {
            SERVER_IP_ADDRESS = strdup(value->valuestring);
        }
        else if (strcmp(key, "SourcePortNumberUDP") == 0) {
            SOURCE_PORT_NUMBER_UDP = strdup(value->valuestring);
        }
        else if (strcmp(key, "DestinationPortNumberUDP") == 0) {
            DESTINATION_PORT_NUMBER_UDP = strdup(value->valuestring);
        }
        else if (strcmp(key, "DestinationPortNumberTCPHeadSYN") == 0) {
            DESTINATION_PORT_NUMBER_TCP_HEAD_SYN = strdup(value->valuestring);
        }
        else if (strcmp(key, "DestinationPortNumberTCPTailSYN") == 0) {
            DESTINATION_PORT_NUMBER_TCP_TAIL_SYN = strdup(value->valuestring);
        }
        else if (strcmp(key, "PortNumberTCP_PreProbingPhases") == 0) {
            PORT_NUMBER_TCP_PRE_PROBING_PHASES = strdup(value->valuestring);
        }
        else if (strcmp(key, "PortNumberTCP_PostProbingPhases") == 0) {
            PORT_NUMBER_TCP_POST_PROBING_PHASES = strdup(value->valuestring);
        }
        else if (strcmp(key, "SizeOfUDPPayloadInBytes") == 0) {
            SIZE_OF_UDP_PAYLOAD_IN_BYTES = strdup(value->valuestring);
        }
        else if (strcmp(key, "InterMeasurementTimeInSeconds") == 0) {
            INTER_MEASUREMENT_TIME_IN_SECONDS = strdup(value->valuestring);
        }
        else if (strcmp(key, "NumberOfUDPPacketsInPacketTrain") == 0) {
            NUMBER_OF_UDP_PACKETS_IN_PACKET_TRAIN = strdup(value->valuestring);
        }
        else if (strcmp(key, "TTLForUDPPackets") == 0) {
            TTL_FOR_UDP_PACKETS = strdup(value->valuestring);
        }

        current_item = current_item->next;
    }

    // Clean up cJSON objects and free the JSON data string
    cJSON_Delete(root);
    free(json_data);
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
    sleep(atoi(INTER_MEASUREMENT_TIME_IN_SECONDS));

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
	asign_from_json(argv[1]);
	//TCP and JSON
	printf("Sending JSON...");
	send_json_over_tcp(argv[1]);
	printf("JSON sent, Sending low packets...");
	send_udp_packets(0);
	printf("low packets sent, waiting 15 secs....");
	
	// recieve from server if there was compression
    
	
    return 0;
}
