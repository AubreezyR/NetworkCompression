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
}

void send_json_over_tcp(char* jsonFile) {
    int s;
   	int status;
   	struct addrinfo hints;
   	struct addrinfo *servinfo;
   	char* ip = cJSON_GetObjectItem(json_dict, "ServerIPAddress")->valuestring;
   	//char* port = cJSON_GetObjectItem(json_dict, "SourcePortNumberUDP")->valuestring;
   	int portInt = cJSON_GetObjectItem(json_dict, "PortNumberTCP_PreProbingPhases")->valueint;
   	char port[5];
   	sprintf(port, "%d", portInt);
   	
   	   	
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // ipv4 or v6 AF_INET is v4 AF_INET6 is v6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;//asigns locall host ip address to socket
    if((status = getaddrinfo(ip, port, &hints, &servinfo)) != 0){//replace NULL with an actuall website or IP if you want
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
    int sleep_time = cJSON_GetObjectItem(json_dict, "InterMeasurementTimeInSeconds")->valueint;

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
    sleep(sleep_time);

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
	//send_udp_packets(0);
	printf("low packets sent, waiting 15 secs....");
	
	// recieve from server if there was compression
    
	
    return 0;
}
