#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//#include "cJSON.h"


#define PORT 8080
#define MAX_JSON_SIZE 4096
#define SERVER_IP "192.168.128.3"
#define PACKET_SIZE 64         // Change to the desired packet size
#define NUM_PACKETS 10         // Number of packets in each phase
#define INTER_MEASUREMENT_TIME 5 // Inter-Measurement Time in seconds

int main(int argc, char *argv[]) {
	if(argc !=2 ){
		printf("Error: Incorrect amount of args");
	}

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
	//FILE *fp 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Communication code goes here...

    //SEND CONFIG///
	// Read the JSON file into a buffer
    char json_buffer[MAX_JSON_SIZE];
    FILE *json_file = fopen(argv[1], "r");
    if (json_file == NULL) {
        perror("Error opening JSON file");
        return EXIT_FAILURE;
    }

    size_t bytesRead = fread(json_buffer, 1, sizeof(json_buffer), json_file);
    fclose(json_file);

    // Send the JSON data to the server
    ssize_t bytesSent = send(sockfd, json_buffer, bytesRead, 0);
    if (bytesSent == -1) {
        perror("Error sending JSON data to server");
        close(sockfd);
        return EXIT_FAILURE;
    }

    //SEND UDP PACKETS//

    char packet[PACKET_SIZE];

    // Phase 1: Send UDP packets with low entropy data
    printf("Phase 1: Sending UDP packets with low entropy data...\n");
    for (int i = 0; i < NUM_PACKETS; i++) {
        // Fill the packet with low entropy data (modify as needed)
        snprintf(packet, sizeof(packet), "Low Entropy Packet %d", i + 1);

        // Send the packet to the server
        sendto(sockfd, packet, strlen(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        printf("Sent: %s\n", packet);

        // Add a delay if desired between low entropy packets (e.g., 100 milliseconds)
        usleep(100000); // Sleep for 100 milliseconds
    }

    // Inter-Measurement Time (γ)
    printf("Inter-Measurement Time (γ): Waiting for %d seconds...\n", INTER_MEASUREMENT_TIME);
    sleep(INTER_MEASUREMENT_TIME);

    // Phase 2: Send UDP packets with high entropy data
    printf("Phase 2: Sending UDP packets with high entropy data...\n");
    for (int i = 0; i < NUM_PACKETS; i++) {
        // Fill the packet with high entropy data (modify as needed)
        snprintf(packet, sizeof(packet), "High Entropy Packet %d", i + 1);

        // Send the packet to the server
        sendto(sockfd, packet, strlen(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        printf("Sent: %s\n", packet);

        // Add a delay if desired between high entropy packets (e.g., 100 milliseconds)
        usleep(100000); // Sleep for 100 milliseconds
    }
	

    
	printf("\nconnection successful. Closing socket\n");
    close(sockfd);
    return 0;
}
