#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SERVER_IP "192.168.128.3" 
#define SERVER_TCP_PORT 8080     
#define SERVER_UDP_PORT 8765  
#define SOURCE_PORT_UDP 9876   

void send_json_over_tcp(char* jsonFile) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create a TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_TCP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to the server over TCP
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Read and send the JSON file over TCP
    FILE *json_file = fopen(jsonFile, "r");
    if (json_file == NULL) {
        perror("Error opening JSON file");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), json_file)) > 0) {
        send(sockfd, buffer, bytesRead, 0);
    }

    fclose(json_file);
    close(sockfd);
}

void send_udp_packets() {
    int sockfd;
    struct sockaddr_in server_addr , client_addr;

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
    client_addr.sin_addr.s_addr = INADDR_ANY;       // Let the system choose the source IP

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
    for (int i = 0; i < 5; i++) {
        char packet[64];
        snprintf(packet, sizeof(packet), "Low Entropy Packet %d", i + 1);
        sendto(sockfd, packet, strlen(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        printf("Sent UDP: %s\n", packet);
        usleep(100000); // Sleep for 100 milliseconds between packets
    }

    close(sockfd);
}

int main(int argc, char *argv[]) {
	if(argc != 2){
		printf("Error: Incorrect number of arguments");
		return EXIT_FAILURE;
	}
    send_json_over_tcp(argv[1]);
    send_udp_packets();
    return 0;
}
