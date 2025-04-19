#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>  // For inet_pton
#include <netdb.h>

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char *message = "Hello from container1";

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Use the container name as the address (DNS resolution in Docker)
    const char* container_name = "server";  // Name of the server container
    struct hostent *host = gethostbyname(container_name);  // Resolve the container name to IP

    if (host == NULL) {
        std::cerr << "Failed to resolve container name" << std::endl;
        return -1;
    }

    // Copy the resolved IP address into the sockaddr_in structure
    memcpy(&serv_addr.sin_addr, host->h_addr_list[0], host->h_length);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // Send message to the server
    send(sock, message, strlen(message), 0);
    std::cout << "Message sent to container2\n";

    // Close the socket
    close(sock);

    return 0;
}
