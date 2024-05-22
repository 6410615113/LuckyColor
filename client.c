#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>

#define SERVER_PORT_1 15113
#define SERVER_PORT_2 25113
#define MAX_MSG 100

// Function to convert a string to lowercase
void toLowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

int main(int argc, char const *argv[]) {
    int client_fd;
    struct sockaddr_in serv_addr_1, serv_addr_2;

    // create socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        printf("Error: socket creation failed\n");
        return 1;
    }

    // convert IP address to binary
    serv_addr_1.sin_family = AF_INET;
    serv_addr_1.sin_port = htons(SERVER_PORT_1);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr_1.sin_addr) <= 0) {
        printf("Error: invalid address\n");
        return 1;
    }

    serv_addr_2.sin_family = AF_INET;
    serv_addr_2.sin_port = htons(SERVER_PORT_2);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr_2.sin_addr) <= 0) {
        printf("Error: invalid address\n");
        return 1;
    }

    // Attempt to connect to SERVER_PORT_1
    if (connect(client_fd, (struct sockaddr *)&serv_addr_1, sizeof(serv_addr_1)) < 0) {
        printf("Error: connection to SERVER_PORT_1 failed\n");
        printf("Attempting to connect to SERVER_PORT_2...\n");

        // Attempt to connect to SERVER_PORT_2
        if (connect(client_fd, (struct sockaddr *)&serv_addr_2, sizeof(serv_addr_2)) < 0) {
            printf("Error: connection to SERVER_PORT_2 failed\n");
            close(client_fd);
            return 1;
        } else {
            printf("Connected to SERVER_PORT_2.\n");
        }
    } else {
        printf("Connected to SERVER_PORT_1.\n");
    }

    while (1) {
        // prompt user to input the day
        printf("Enter luck ('man' to see the manual, 'chi' to get a random number): ");
        char input[MAX_MSG];
        fgets(input, MAX_MSG, stdin);
        // remove trailing newline character
        input[strcspn(input, "\n")] = '\0';

        // Convert the input to lowercase for case-insensitive comparison
        char lower_input[MAX_MSG];
        strcpy(lower_input, input);
        toLowercase(lower_input);

        if (strcmp(lower_input, "man") == 0) {
            // Send "man" command to the server
            send(client_fd, "man", strlen("man"), 0);

            // Receive and print the manual from the server
            char manual[MAX_MSG * 10] = {0}; // Initialize the receive buffer for manual
            int bytes_received = recv(client_fd, manual, sizeof(manual), 0);
            if (bytes_received < 0) {
                printf("Error: recv failed\n");
                break;
            } else if (bytes_received == 0) {
                printf("Server closed the connection\n");
                break;
            }
            printf("%s\n", manual); // Print the manual
            continue;
        }

        if (strcmp(lower_input, "bye") == 0) {
            break; // Exit the loop to terminate the connection
        }

        // send the input to the server
        if (send(client_fd, input, strlen(input), 0) < 0) {
            printf("Error: send failed\n");
            break;
        }

        // Receive the response from the server
        char rcv_msg[MAX_MSG * 10] = {0}; // Initialize the receive buffer
        int bytes_received = recv(client_fd, rcv_msg, sizeof(rcv_msg), 0);
        if (bytes_received < 0) {
            printf("Error: recv failed\n");
            break;
        } else if (bytes_received == 0) {
            printf("Server closed the connection\n");
            break;
        }

        // Check if the received message indicates a wrong input
        if (strcmp(rcv_msg, "Wrong input!, please try again") == 0) {
            printf("Error: Wrong input received from the server.\n");
        } else {
            if (strcmp(lower_input, "chi") == 0) {
                printf("Chi-chi stick: \n");
                printf("------\n");
                printf("%s", rcv_msg);
                printf("------\n");
            } else {
                // Print the received response in table format
                printf("---------------------------------------------------\n");
                printf("|     Day     |      Color      |       Work      |\n");
                printf("---------------------------------------------------\n");
                printf("%s", rcv_msg);
                printf("---------------------------------------------------\n");
            }
        }
    }

    // Inform the server about the "bye" message and close socket
    send(client_fd, "bye", strlen("bye"), 0);
    close(client_fd);

    return 0;
}
