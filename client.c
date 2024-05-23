#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>

#define SERVER_PORT_1 15113
#define SERVER_PORT_2 25113
#define MAX_MSG 1024

void toLowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void printChiTextInLines(char *chi_text) {
    char *word;
    int word_count = 0;
    char line[MAX_MSG] = {0};
    int line_length = 0;

    word = strtok(chi_text, " ");
    while (word != NULL) {
        int word_length = strlen(word);
        if (line_length + word_length > 30) {
            printf(" %s\n", line);
            strcpy(line, "");
            line_length = 0;
            word_count = 0;
        }
        if (word_count > 0) {
            strcat(line, " ");
            line_length++;
        }
        strcat(line, word);
        line_length += word_length;
        word_count++;
        word = strtok(NULL, " ");
    }
    if (word_count > 0) {
        printf(" %s\n", line);
    }
}

int main(int argc, char const *argv[]) {
    int client_fd;
    struct sockaddr_in serv_addr_1, serv_addr_2;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        printf("Error: socket creation failed\n");
        return 1;
    }

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

    if (connect(client_fd, (struct sockaddr *)&serv_addr_1, sizeof(serv_addr_1)) < 0) {
        printf("Error: connection to SERVER_PORT_1 failed\n");
        printf("Attempting to connect to SERVER_PORT_2...\n");

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
        printf("Enter luck ('man' - To show commands): ");
        char input[MAX_MSG];
        fgets(input, MAX_MSG, stdin);
        input[strcspn(input, "\n")] = '\0';

        toLowercase(input);

        if (strcmp(input, "man") == 0 || strcmp(input, "chi") == 0) {
            send(client_fd, input, strlen(input), 0);
            char response[MAX_MSG] = {0};
            int bytes_received = recv(client_fd, response, sizeof(response), 0);
            if (bytes_received < 0) {
                printf("Error: recv failed\n");
                break;
            } else if (bytes_received == 0) {
                printf("Server closed the connection\n");
                break;
            }

            if (strcmp(input, "chi") == 0) {
                // Parse the response to extract random number and chi_text
                int random_number;
                char chi_text[MAX_MSG];
                sscanf(response, "%d|%[^\n]", &random_number, chi_text);

                // Display the random number
                printf("Chi-chi stick:\n");
                printf("------\n");
                printf("| %d |\n", random_number);
                printf("------\n");

                // Display the matched line from chi.csv
                printf("--------------------------------\n");
                printChiTextInLines(chi_text);
                printf("--------------------------------\n\n");
            } else {
                printf("%s\n", response);
            }

            continue;
        }

        if (strcmp(input, "bye") == 0) {
            send(client_fd, "bye", strlen("bye"), 0);
            break;
        }

        send(client_fd, input, strlen(input), 0);

        char rcv_msg[MAX_MSG] = {0};
        int bytes_received = recv(client_fd, rcv_msg, sizeof(rcv_msg), 0);
        if (bytes_received < 0) {
            printf("Error: recv failed\n");
            break;
        } 
        
        if (bytes_received == 0) {
            printf("Server closed the connection\n");
            break;
        }

        if (strcmp(rcv_msg, "Wrong input! Please try again.") == 0) {
            printf("%s\n", rcv_msg);
        } else {
            printf("---------------------------------------------------------------------------------------------------------------------------------------\n");
            printf("|     Day     |      Kindness     |        Work       |       Lucky       |       Money       |       Lovely      |     Don't use     |\n");
            printf("---------------------------------------------------------------------------------------------------------------------------------------\n");
            printf("%s", rcv_msg);
            printf("---------------------------------------------------------------------------------------------------------------------------------------\n\n");
        }
    }

    close(client_fd);

    return 0;
}
