#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h> // for threading support
#include <time.h>     // for date and time functions

#define SERVER_PORT_1 15113
#define SERVER_PORT_2 25113
#define MAX_MSG 1024
#define GMT_OFFSET 7 // GMT+7

char *color_table[7] = {"yellow", "pink", "green", "orange", "blue", "purple", "red"};
char *days[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
char *work[7] = {"Green", "Pink, Gray", "Orange, Gold", "Skyblue", "Yellow, White", "Silver, Brown", "Pink"};
char *abbreviations[][2] = {
    {"mon", "Monday"},
    {"tue", "Tuesday"},
    {"wed", "Wednesday"},
    {"thu", "Thursday"},
    {"fri", "Friday"},
    {"sat", "Saturday"},
    {"sun", "Sunday"},
    {"ytd", "yesterday"},
    {"tmr", "tomorrow"},
    {"td", "today"}};

// Function to convert a string to lowercase
void toLowercase(char *str)
{
    for (int i = 0; str[i]; i++)
    {
        str[i] = tolower(str[i]);
    }
}

// Function to map abbreviation or day name variations to full name
const char *mapAbbreviation(char *input)
{
    // Convert the input to lowercase for case-insensitive comparison
    toLowercase(input);

    // Check if the input matches any abbreviation or day name variation
    for (int i = 0; i < sizeof(abbreviations) / sizeof(abbreviations[0]); ++i)
    {
        if (strcmp(input, abbreviations[i][0]) == 0 || strcmp(input, abbreviations[i][1]) == 0)
        {
            return abbreviations[i][1]; // Return the full day name
        }
    }

    // Check if the input matches any full day names
    for (int i = 0; i < sizeof(days) / sizeof(days[0]); ++i)
    {
        if (strcmp(input, days[i]) == 0)
        {
            return days[i]; // Return the full day name
        }
    }

    return input; // Return the original input if no match found
}

// Function to find the index of a day in the days array
int findDayIndex(char *day)
{
    for (int i = 0; i < 7; ++i)
    {
        if (strcmp(day, days[i]) == 0)
        {
            return i;
        }
    }
    return -1; // Day not found
}

// Function to get the current day of the week in GMT+7 timezone
int getCurrentDayIndex()
{
    time_t rawtime;
    struct tm *tm_info;
    setenv("TZ", "Asia/Bangkok", 1); // Set the timezone to GMT+7 (Bangkok, Thailand)
    tzset();                          // Update the timezone
    time(&rawtime);
    tm_info = localtime(&rawtime); // Fetch local time in GMT+7
    int dayIndex = tm_info->tm_wday; // Sunday is 0, Monday is 1, ..., Saturday is 6
    // Adjust for timezone offset
    dayIndex = (dayIndex + GMT_OFFSET) % 7;
    return dayIndex;
}

// Thread function to generate a random number between 1 and 30
void *getRandomNumber(void *arg)
{
    int *client_socket = (int *)arg;
    char response[MAX_MSG] = {0};

    // Generate a random number between 1 and 30
    int random_number = (rand() % 30) + 1;

    // Format the random number to always be two digits
    sprintf(response, "| %02d |\n", random_number);

    // Send the formatted random number to the client
    send(*client_socket, response, strlen(response), 0);

    return NULL; // Exit thread
}


// Thread function to handle client requests
void *clientHandler(void *arg)
{
    int client_socket = *((int *)arg);
    free(arg); // Free the allocated memory for client socket

    while (1)
    {
        char rcv_day[MAX_MSG] = {0};
        int bytes_read = read(client_socket, rcv_day, MAX_MSG);
        if (bytes_read <= 0)
        {
            printf("Client disconnected or error occurred\n");
            break;
        }

        // Remove trailing newline character if any
        rcv_day[strcspn(rcv_day, "\n")] = '\0';

        printf("Received day from client: %s\n", rcv_day);

        // Convert the received day to lowercase for case-insensitive comparison
        toLowercase(rcv_day);

        // Map abbreviation to full name
        const char *full_day = mapAbbreviation(rcv_day);

        if (strcmp(full_day, "bye") == 0)
        {
            printf("Client sent 'bye'. Closing connection.\n");
            break;
        }
        else if (strcmp(full_day, "man") == 0)
        {
            // Send manual/instructions to the client
            char manual[] = "Welcome to Lucky Service!\n"
                            "'mon', 'tue', ..., 'sun' - To get Lucky Color about Specific Days.\n"
                            "'td' - To get Lucky Color about the current day.\n"
                            "'ytd' - To get Lucky Color about the previous day.\n"
                            "'tmr' - To get Lucky Color about the next day.\n"
                            "'all' - To get Lucky Color about all days.\n"
                            "'chi' - To get a random Chi-chi stick.\n"
                            "'bye' - Exit.\n";
            send(client_socket, manual, strlen(manual), 0);
            continue;
        }
        else if (strcmp(full_day, "chi") == 0)
        {
            // Create a new thread to generate a random number between 01 and 30
            pthread_t random_thread;
            int *client_socket_copy = malloc(sizeof(int));
            if (client_socket_copy == NULL)
            {
                printf("Error: could not allocate memory for client socket copy\n");
                break;
            }
            *client_socket_copy = client_socket; // Make a copy of the client socket

            pthread_create(&random_thread, NULL, getRandomNumber, client_socket_copy);
            pthread_detach(random_thread);
            continue;
        }

        // Check for special cases: today, yesterday, tomorrow, or all
        int dayIndex = -1;
        if (strcmp(full_day, "today") == 0)
        {
            // Get the current day of the week
            dayIndex = getCurrentDayIndex();
        }
        else if (strcmp(full_day, "yesterday") == 0)
        {
            // Get the index of the day before today
            dayIndex = (getCurrentDayIndex() + 6) % 7;
        }
        else if (strcmp(full_day, "tomorrow") == 0)
        {
            // Get the index of the day after today
            dayIndex = (getCurrentDayIndex() + 1) % 7;
        }
        else if (strcmp(full_day, "all") == 0)
        {
            // Construct the response in table format
            char response[MAX_MSG * 10] = {0}; // Increased buffer size
            int offset = 0;

            for (int i = 0; i < 7; ++i)
            {
                offset += sprintf(response + offset, "| %-11s | %-15s | %-15s |\n", days[i], color_table[i], work[i]);
            }

            // Send the response to the client
            send(client_socket, response, strlen(response), 0);
            continue; // Continue to wait for the next input
        }
        else
        {
            // Find the index of the received day in the days array
            dayIndex = findDayIndex(full_day);
        }

        // Prepare the response
        char response[MAX_MSG] = {0};
        if (dayIndex != -1)
        {
            // Construct the response with color and index information
            sprintf(response, "| %-11s | %-15s | %-15s |\n", days[dayIndex], color_table[dayIndex], work[dayIndex]);
        }
        else
        {
            // Invalid input
            strcpy(response, "Wrong input!, Please try again");
        }

        // Send the response to the client
        send(client_socket, response, strlen(response), 0);
    }

    // Close the client socket
    close(client_socket);
    return NULL; // Exit thread
}

int main(int argc, char const *argv[])
{
    srand(time(NULL)); // Seed the random number generator

    int serv_fd;
    struct sockaddr_in serv_addr_1, serv_addr_2;

    // create socket
    serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd < 0)
    {
        printf("Error: socket creation failed\n");
        return 1;
    }

    // Attempt to bind serv_fd to SERVER_PORT_1
    serv_addr_1.sin_family = AF_INET;
    serv_addr_1.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr_1.sin_port = htons(SERVER_PORT_1);

    if (bind(serv_fd, (struct sockaddr *)&serv_addr_1, sizeof(serv_addr_1)) < 0)
    {
        printf("Error: bind SERVER_PORT_1 failed\n");
        printf("Attempting to bind to SERVER_PORT_2...\n");

        // Attempt to bind serv_fd to SERVER_PORT_2
        serv_addr_2.sin_family = AF_INET;
        serv_addr_2.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr_2.sin_port = htons(SERVER_PORT_2);
        if (bind(serv_fd, (struct sockaddr *)&serv_addr_2, sizeof(serv_addr_2)) < 0)
        {
            printf("Error: bind SERVER_PORT_2 failed. Cannot start the server.\n");
            close(serv_fd);
            return 1;
        }
        else
        {
            printf("Successfully bound to SERVER_PORT_2.\n");
        }
    }
    else
    {
        printf("Server bound to SERVER_PORT_1.\n");
    }

    // listen for new connections
    if (listen(serv_fd, 5) < 0)
    {
        printf("Error: listen failed\n");
        close(serv_fd);
        return 1;
    }

    // Server loop
    while (1)
    {
        int *client_socket = malloc(sizeof(int));
        if (client_socket == NULL)
        {
            printf("Error: could not allocate memory for client socket\n");
            continue;
        }

        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        // accept new connection
        *client_socket = accept(serv_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (*client_socket < 0)
        {
            printf("Error: accept failed\n");
            free(client_socket);
            continue;
        }

        // Create a thread to handle the client request
        pthread_t thread;
        pthread_create(&thread, NULL, clientHandler, client_socket);
        pthread_detach(thread);
    }

    // Close server socket
    close(serv_fd);
    return 0;
}

