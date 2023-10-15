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
#define SERVER_IP "192.168.128.3" 
#define SERVER_TCP_PORT 8080     
#define SERVER_UDP_PORT 8765  
#define SOURCE_PORT_UDP 9876
#define PACKET_SIZE 1400     
#define PACKET_COUNT 5      
#define THRESHOLD 100
#define WAIT_TIME 100      
//TODO READ DATA FROM JSON INSTEAD OF HARDCODING IT

void send_json_over_tcp(char* jsonFile) {
    int s;
   	int status;
   	struct addrinfo hints;
   	struct addrinfo *servinfo;
   
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // ipv4 or v6 AF_INET is v4 AF_INET6 is v6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;//asigns locall host ip address to socket

    if((status = getaddrinfo("192.168.128.3", "8080", &hints, &servinfo)) != 0){//replace NULL with an actuall website or IP if you want
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
/*
void send_udp_packets(int payload_type) {
    int sockfd;
    struct sockaddr_in server_addr , client_addr;
	char packet[PACKET_SIZE];
	
    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }
    
	memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_UDP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Set the source port for UDP
    memset(&client_addr, 0, sizeof(client_addr));
    
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(SOURCE_PORT_UDP); // Set the desired source UDP port

    // Bind the socket to the specified source port for UDP
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Binding source port for UDP failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

	//Set the DOnt Fragment flag in IP header
	int enable = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &enable, sizeof(enable)) < 0) {
        perror("Failed to set DF flag");
        exit(EXIT_FAILURE);
    }

    // Send UDP packets (low entropy data)
    for (int i = 0; i < PACKET_COUNT; i++) {
    	memset(packet, 0, sizeof(packet)); // Initialize packet with all 0's
    	
    	if(payload_type == 1){
		    //Fill packet wil random data
		    FILE *urandom = fopen("/dev/urandom", "rb");
		    fread(packet, 1, sizeof(packet), urandom);
		    fclose(urandom);
	    }
	    sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
	    sleep(15000);
    }

    close(sockfd);
}*/


void send_udp_packets(int packet_type) {
    int s;
   	int status;
   	struct addrinfo hints;
   	struct addrinfo *servinfo;
   
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // ipv4 or v6 AF_INET is v4 AF_INET6 is v6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;//asigns locall host ip address to socket

    if((status = getaddrinfo("192.168.128.3", "8080", &hints, &servinfo)) != 0){//replace NULL with an actuall website or IP if you want
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));    
		exit(1);
    }
   	//set up socket
   	s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
   	//bind socket to port
   	/*
   	if(bind(s, servinfo->ai_addr, servinfo->ai_addrlen)< 0){
   		perror("TCP socket bind fail");
   		exit(1);
   	}*/
   	if(connect(s,servinfo->ai_addr, servinfo->ai_addrlen) < 0){
   		perror("Connect error");
   		close(s);
   	}

   	

   // Create the packet
   char packet_load[100]; // Declare a character array
   
   // Fill the string with null characters
   memset(packet_load, '0', sizeof(packet_load));

   // Open /dev/urandom as a source of randomness if we are doing a high packet
   if(packet_type){
	   int urandom_fd = open("/dev/urandom", O_RDONLY);
	   if (urandom_fd == -1) {
	       perror("open /dev/urandom");
	       exit(1);
	   }

	   // Read random data from /dev/urandom to replace null characters
	   if (read(urandom_fd, packet_load, sizeof(packet_load)) == -1) {
	       perror("read /dev/urandom");
	       close(urandom_fd);
	       exit(1);
	   }

	   close(urandom_fd);
   }

   // Send the data to the server
   if (sendto(s, packet_load, strlen(packet_load), 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
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
	sleep(WAIT_TIME);
	printf("Sleep over, now sending high packets...");
	send_udp_packets(1);
	// recieve from server if there was compression
    
	
    return 0;
}
