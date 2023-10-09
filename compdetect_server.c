#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080  // Port number for the server to listen on
#define MAX_JSON_SIZE 4096
int main() {
    int server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1) { // The second argument is the maximum queue length for pending connections
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept incoming connections
    new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (new_socket == -1) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // You can now communicate with the client through the "new_socket" descriptor
  	// Receive the JSON data from the client
    char json_buffer[MAX_JSON_SIZE];
    ssize_t bytesRead = recv(new_socket, json_buffer, sizeof(json_buffer), 0);

    if (bytesRead == -1) {
        perror("Error receiving JSON data from client");
        close(new_socket);
        return EXIT_FAILURE;
    }

    // Write the received JSON data to a file
    FILE *json_file = fopen("received_data.json", "w");
    if (json_file == NULL) {
        perror("Error opening file for writing");
        close(new_socket);
        return EXIT_FAILURE;
    }

    size_t bytesWritten = fwrite(json_buffer, 1, bytesRead, json_file);
    fclose(json_file);

    printf("Received JSON data and saved it to received_data.json\n");

    close(new_socket); // Close the connection
    close(server_fd);  // Close the server socket

    return 0;
}
