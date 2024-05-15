#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT_1 15113
#define SERVER_PORT_2 25113
#define MAX_MSG 100

int main(int argc, char const* argv[]) {
    int client_fd_1, client_fd_2;
    struct sockaddr_in serv_addr_1, serv_addr_2;

    // create socket
    client_fd_1 = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd_1 < 0) {
        printf("Error: socket SERVER 1 failed");
        return 1;
    }

    client_fd_2 = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd_2 < 0) {
        printf("Error: socket SERVER 2 failed");
        return 1;
    }

    // convert IP address to binary
    serv_addr_1.sin_family = AF_INET;
    serv_addr_1.sin_port = htons(SERVER_PORT_1);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr_1.sin_addr) <= 0) {
        printf("Error: invalid address");
        return 1;
    }

    serv_addr_2.sin_family = AF_INET;
    serv_addr_2.sin_port = htons(SERVER_PORT_2);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr_2.sin_addr) <= 0) {
        printf("Error: invalid address");
        return 1;
    }

    // connect socket to SERVER_PORT_1
    if (connect(client_fd_1, (struct sockaddr *) &serv_addr_1, sizeof(serv_addr_1)) < 0) {
        printf("Error: connection to SERVER_PORT_1 failed\n");
        printf("Attempting to connect to SERVER_PORT_2...\n");

        // connect socket to SERVER_PORT_2
        if (connect(client_fd_2, (struct sockaddr *) &serv_addr_2, sizeof(serv_addr_2)) < 0) {
            printf("Error: connection to SERVER_PORT_2 failed\n");
            return 1;
        } else {
            printf("Connected to SERVER_PORT_2.\n");
        }
    } else {
        printf("Connected to SERVER_PORT_1.\n");
    }


    // prompt user to input the day
    printf("Enter the day (e.g., Monday, Tuesday, etc.): ");
    char day_input[MAX_MSG];
    fgets(day_input, MAX_MSG, stdin);
    // remove trailing newline character
    day_input[strcspn(day_input, "\n")] = '\0';

    // send the input day to the server via SERVER_PORT_1
    send(client_fd_1, day_input, strlen(day_input), 0);

    // Receive the response from the server
    char rcv_msg[MAX_MSG * 10];
    recv(client_fd_1, rcv_msg, sizeof(rcv_msg), 0);

    // Check if the received message indicates a wrong input
    if (strcmp(rcv_msg, "Wrong input!, please try again") == 0) {
        printf("Error: Wrong input received from the server.\n");
        // You can take further action here, such as requesting input again from the user
    } else {
        // Print the received response in table format
        printf("__________________________________________________\n");
        printf("|     Day     |      Color      |       Work      |\n");
        printf("---------------------------------------------------\n");
        printf("%s", rcv_msg);
        printf("---------------------------------------------------\n");
    }
    // close sockets
    close(client_fd_1);
    close(client_fd_2);

    return 0;
}
