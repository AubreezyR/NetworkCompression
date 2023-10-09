#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//#include "cJSON.h"


#define PORT 8080

int main(int argc, char *argv[]) {
	if(argc !=2 ){
		printf("Error: Incorrect amount of args");
	}

	// Open the JSON file for reading
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Unable to open the JSON file\n");
        return 1;
    }

    // Read and print the JSON file contents
    int c;
    while ((c = fgetc(file)) != EOF) {
        putchar(c);
    }

    // Close the file
    fclose(file);
	
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
	printf("\nconnection successful. Closing socket\n");
    close(sockfd);
    return 0;
}
