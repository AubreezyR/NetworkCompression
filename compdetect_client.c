#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//#include "cJSON.h"


#define PORT 8080
#define MAX_JSON_SIZE 4096

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
    server_addr.sin_addr.s_addr = inet_addr("192.168.128.3");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Communication code goes here...
	// Read the JSON file into a buffer
    char json_buffer[MAX_JSON_SIZE];
    FILE *json_file = fopen("data.json", "r");
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
	

    
	printf("\nconnection successful. Closing socket\n");
    close(sockfd);
    return 0;
}
