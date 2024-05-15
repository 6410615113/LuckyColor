#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h> // for threading support
#include <time.h> // for date and time functions

#define SERVER_PORT_1 15113
#define SERVER_PORT_2 25113
#define MAX_MSG 1024
#define GMT_OFFSET 7 // GMT+7

char *color_table[7] = {"yellow", "pink", "green", "orange", "blue", "purple", "red"};
char *days[7] = {"monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"};
char *work[7] = {"Green", "Pink, Gray", "Orange, Gold", "Skyblue", "Yellow, White", "Silver, Brown", "Pink"};

// Function to convert a string to lowercase
void toLowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Function to find the index of a day in the days array
int findDayIndex(char *day) {
    for (int i = 0; i < 7; ++i) {
        if (strcmp(day, days[i]) == 0) {
            return i;
        }
    }
    return -1; // Day not found
}

// Function to get the current day of the week in GMT+7 timezone
int getCurrentDayIndex() {
    time_t rawtime;
    struct tm *tm_info;
    setenv("TZ", "Asia/Bangkok", 1); // Set the timezone to GMT+7 (Bangkok, Thailand)
    tzset(); // Update the timezone
    time(&rawtime);
    tm_info = localtime(&rawtime); // Fetch local time in GMT+7
    int dayIndex = tm_info->tm_wday; // Sunday is 0, Monday is 1, ..., Saturday is 6
    // Adjust for timezone offset
    dayIndex = (dayIndex + GMT_OFFSET) % 7;
    return dayIndex;
}

// Thread function to handle client requests
void *clientHandler(void *arg) {
    int client_socket = *((int *)arg);

    char rcv_day[MAX_MSG] = {0};
    read(client_socket, rcv_day, MAX_MSG);
    printf("Received day from client: %s\n", rcv_day);

    // Convert the received day to lowercase for case-insensitive comparison
    toLowercase(rcv_day);

    // Check for special cases: today, yesterday, tomorrow, or all
    int dayIndex = -1;
    if (strcmp(rcv_day, "today") == 0) {
        // Get the current day of the week
        dayIndex = getCurrentDayIndex();
    } else if (strcmp(rcv_day, "yesterday") == 0) {
        // Get the index of the day before today
        dayIndex = (getCurrentDayIndex() + 6) % 7;
    } else if (strcmp(rcv_day, "tomorrow") == 0) {
        // Get the index of the day after today
        dayIndex = (getCurrentDayIndex() + 1) % 7;
    } else if (strcmp(rcv_day, "all") == 0) {
        // Construct the response in table format
        char response[MAX_MSG * 10] = {0}; // Increased buffer size
        int offset = 0;
    
        for (int i = 0; i < 7; ++i) {
            offset += sprintf(response + offset, "| %-11s | %-15s | %-15s |\n", days[i], color_table[i], work[i]);
        }
        

        // Send the response to the client
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        return NULL; // Exit thread
    } else {
        // Find the index of the received day in the days array
        dayIndex = findDayIndex(rcv_day);
    }

    // Prepare the response
    char response[MAX_MSG] = {0};
    if (dayIndex != -1) {
        // Construct the response with color and index information
        sprintf(response, "| %-11s | %-15s | %-15s |\n", days[dayIndex], color_table[dayIndex], work[dayIndex]);
    } else {
        // Invalid input
        strcpy(response, "Wrong input!, please try again");
    }

    // Send the response to the client
    send(client_socket, response, strlen(response), 0);
    close(client_socket);
    return NULL; // Exit thread
}

int main(int argc, char const *argv[]) {
    int serv_fd_1, serv_fd_2;
    struct sockaddr_in serv_addr_1, serv_addr_2;

    // create socket
    serv_fd_1 = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd_1 < 0) {
        printf("Error: socket SERVER 1 failed");
        return 1;
    }

    serv_fd_2 = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd_2 < 0) {
        printf("Error: socket SERVER 2 failed");
        return 1;
    }

    // Attempt to bind serv_fd_1 to SERVER_PORT_1
    serv_addr_1.sin_family = AF_INET;
    serv_addr_1.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr_1.sin_port = htons(SERVER_PORT_1);

    if (bind(serv_fd_1, (struct sockaddr *)&serv_addr_1, sizeof(serv_addr_1)) < 0) {
        printf("Error: bind SERVER_PORT_1 failed\n");
        printf("Attempting to bind to SERVER_PORT_2...\n");

        // Attempt to bind serv_fd_1 to SERVER_PORT_2
        serv_addr_2.sin_family = AF_INET;
        serv_addr_2.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr_2.sin_port = htons(SERVER_PORT_2);
        if (bind(serv_fd_1, (struct sockaddr *)&serv_addr_2, sizeof(serv_addr_2)) < 0) {
            printf("Error: bind SERVER_PORT_2 failed. Cannot start the server.\n");
            return 1;
        } else {
            // If successfully bound to SERVER_PORT_2, update serv_fd_1 to use SERVER_PORT_2
            printf("Successfully bound to SERVER_PORT_2.\n");
        }
    } else {
        printf("Server bound to SERVER_PORT_1.\n");
    }

    // listen for new connection
    if (listen(serv_fd_1, 5) < 0) {
        printf("Error: listen SERVER_PORT_1 failed");
        return 1;
    }

    if (listen(serv_fd_2, 5) < 0) {
        printf("Error: listen SERVER_PORT_2 failed");
        return 1;
    }

    // Server loop
    while (1) { 
        int client_socket_1, client_socket_2;
        struct sockaddr_in client_addr_1, client_addr_2;
        int addr_len_1 = sizeof(client_addr_1);
        int addr_len_2 = sizeof(client_addr_2);

        // accept new connection
        client_socket_1 = accept(serv_fd_1, (struct sockaddr *)&client_addr_1, (socklen_t *)&addr_len_1);
        client_socket_2 = accept;

        (serv_fd_2, (struct sockaddr *)&client_addr_2, (socklen_t *)&addr_len_2);

        // Create threads to handle client requests
        pthread_t thread_1, thread_2;
        pthread_create(&thread_1, NULL, clientHandler, &client_socket_1);
        pthread_create(&thread_2, NULL, clientHandler, &client_socket_2);
        pthread_detach(thread_1);
        pthread_detach(thread_2);
    } // End of server loop

    // Close server sockets
    close(serv_fd_1);
    close(serv_fd_2);
    return 0;
}

