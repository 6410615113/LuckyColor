#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define SERVER_PORT_1 15113
#define SERVER_PORT_2 25113
#define MAX_MSG 1024
#define GMT_OFFSET 7 // GMT+7
#define MAX_CHI_LINES 30
#define MAX_CHI_LINE_LENGTH 500

char *days[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
char colors[7][6][50]; // 7 days, 7 columns, 50 characters max per color entry

// Structure to hold Chi-chi stick lines
typedef struct {
    int number;
    char text[MAX_MSG];
} ChiLine;

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
void toLowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Function to map abbreviation or day name variations to full name
const char *mapAbbreviation(char *input) {
    // Convert the input to lowercase for case-insensitive comparison
    toLowercase(input);

    // Check if the input matches any abbreviation or day name variation
    for (int i = 0; i < sizeof(abbreviations) / sizeof(abbreviations[0]); ++i) {
        if (strcmp(input, abbreviations[i][0]) == 0 || strcmp(input, abbreviations[i][1]) == 0) {
            return abbreviations[i][1]; // Return the full day name
        }
    }

    // Check if the input matches any full day names
    for (int i = 0; i < sizeof(days) / sizeof(days[0]); ++i) {
        if (strcmp(input, days[i]) == 0) {
            return days[i]; // Return the full day name
        }
    }

    return input; // Return the original input if no match found
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
    tzset();                          // Update the timezone
    time(&rawtime);
    tm_info = localtime(&rawtime); // Fetch local time in GMT+7
    int dayIndex = tm_info->tm_wday; // Sunday is 0, Monday is 1, ..., Saturday is 6
    // Adjust for timezone offset
    dayIndex = (dayIndex + GMT_OFFSET) % 7;
    return dayIndex;
}

// Function to read the color data from color.csv
void readColorData() {
    FILE *file = fopen("color.csv", "r");
    if (!file) {
        perror("Error opening color.csv");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int row = 0;

    // Skip the header line
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) && row < 7) {
        char *token = strtok(line, ",");
        int col = 0;
        while (token && col < 6) {
            strncpy(colors[row][col], token, 49);
            colors[row][col][49] = '\0'; // Ensure null-termination
            token = strtok(NULL, ",");
            col++;
        }
        row++;
    }

    fclose(file);
}

// Function to read Chi-chi data lines from "chi.csv"
void readChiData(ChiLine chi_lines[MAX_CHI_LINES]) {
    FILE *file = fopen("chi.csv", "r");
    if (!file) {
        perror("Error opening chi.csv");
        exit(EXIT_FAILURE);
    }

    int line_number = 0;
    while (fgets(chi_lines[line_number].text, sizeof(chi_lines[line_number].text), file) && line_number < MAX_CHI_LINES) {
        // Remove newline character
        chi_lines[line_number].text[strcspn(chi_lines[line_number].text, "\n")] = '\0';
        chi_lines[line_number].number = line_number + 1; // Chi-chi stick numbers start from 1
        line_number++;
    }

    fclose(file);
}

// Function to match random number with Chi-chi stick line and return the text
char *getChiText(int random_number) {
    // Read Chi-chi stick lines from "chi.csv"
    static ChiLine chi_lines[MAX_CHI_LINES]; // static to keep data between function calls
    static int data_loaded = 0;

    if (!data_loaded) {
        readChiData(chi_lines);
        data_loaded = 1;
    }

    // Check if the random number is within the valid range
    if (random_number < 1 || random_number > MAX_CHI_LINES) {
        return "Invalid random number";
    }

    // Return the Chi-chi stick text corresponding to the random number
    return chi_lines[random_number - 1].text;
}

// Thread function to generate a random number between 1 and 30 and send it to the client
void *getRandomNumber(void *arg) {
    int client_socket = *(int *)arg;
    char response[MAX_MSG] = {0};

    // Generate a random number between 1 and 30
    int random_number = (rand() % 30) + 1;

    // Get the corresponding chi text
    char *chi_text = getChiText(random_number);

    // Format the response: random_number|chi_text
    snprintf(response, sizeof(response), "%02d|%s", random_number, chi_text);

    // Send the response to the client
    send(client_socket, response, strlen(response), 0);

    return NULL; // Exit thread
}

// Thread function to handle client requests
void *clientHandler(void *arg) {
    int client_socket = *((int *)arg);
    free(arg); // Free the allocated memory for client socket

    while (1) {
        char rcv_day[MAX_MSG] = {0};
        int bytes_read = read(client_socket, rcv_day, MAX_MSG);
        if (bytes_read <= 0) {
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

        if (strcmp(full_day, "bye") == 0) {
            printf("Client sent 'bye'. Closing connection.\n");
            break;
        } else if (strcmp(full_day, "man") == 0) {
            // Send manual/instructions to the client
            char manual[] = "Welcome to Lucky Service!\n"
                            "'man' - To show commands.\n"
                            "'mon', 'tue', ..., 'sun' - To get Lucky Color about Specific Days.\n"
                            "'td' - To get Lucky Color about the current day.\n"
                            "'ytd' - To get Lucky Color about the previous day.\n"
                            "'tmr' - To get Lucky Color about the next day.\n"
                            "'all' - To get Lucky Color about all days.\n"
                            "'chi' - To get a random Chi-chi stick.\n"
                            "'bye' - Exit.\n";
            send(client_socket, manual, strlen(manual), 0);
            continue;
        } else if (strcmp(full_day, "chi") == 0) {
            // Generate a random number between 1 and MAX_CHI_LINES
            pthread_t chi_thread;
            pthread_create(&chi_thread, NULL, getRandomNumber, &client_socket);
            pthread_detach(chi_thread);
            continue;
        }

        // Check for special cases: today, yesterday, tomorrow, or all
        int dayIndex = -1;
        if (strcmp(full_day, "today") == 0) {
            // Get the current day of the week
            dayIndex = (getCurrentDayIndex() + 6) % 7;
        } else if (strcmp(full_day, "yesterday") == 0) {
            // Get the index of the day before today
            dayIndex = (getCurrentDayIndex() + 5) % 7;
        } else if (strcmp(full_day, "tomorrow") == 0) {
            // Get the index of the day after today
            dayIndex = getCurrentDayIndex();
        } else if (strcmp(full_day, "all") == 0) {
            // Construct the response in table format
            char response[MAX_MSG * 10] = {0}; // Increased buffer size
            int offset = 0;

            for (int i = 0; i < 7; ++i) {
                offset += sprintf(response + offset, "| %-11s | %-17s | %-17s | %-17s | %-17s | %-17s | %s",
                                  days[i], colors[i][0], colors[i][1], colors[i][2], colors[i][3], colors[i][4], colors[i][5]);
            }

            // Send the response to the client
            send(client_socket, response, strlen(response), 0);
            continue; // Continue to wait for the next input
        } else {
            // Find the index of the received day in the days array
            dayIndex = findDayIndex(full_day);
        }

        // Prepare the response
        char response[MAX_MSG] = {0};
        if (dayIndex != -1) {
            // Construct the response with color information
            sprintf(response, "| %-11s | %-17s | %-17s | %-17s | %-17s | %-17s | %s",
                    days[dayIndex], colors[dayIndex][0], colors[dayIndex][1], colors[dayIndex][2], colors[dayIndex][3], colors[dayIndex][4], colors[dayIndex][5]);
        } else {
            // Invalid input
            strcpy(response, "Wrong input! Please try again.");
        }

        // Send the response to the client
        send(client_socket, response, strlen(response), 0);
    }
    // Close the client socket
    close(client_socket);
    return NULL; // Exit thread
}

int main(int argc, char const *argv[]) {
    srand(time(NULL)); // Seed the random number generator
    readColorData(); // Read color data from CSV file

    int serv_fd;
    struct sockaddr_in serv_addr_1, serv_addr_2;

    // create socket
    serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd < 0) {
        printf("Error: socket creation failed\n");
        return 1;
    }

    // Attempt to bind serv_fd to SERVER_PORT_1
    serv_addr_1.sin_family = AF_INET;
    serv_addr_1.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr_1.sin_port = htons(SERVER_PORT_1);

    if (bind(serv_fd, (struct sockaddr *)&serv_addr_1, sizeof(serv_addr_1)) < 0) {
        printf("Error: bind SERVER_PORT_1 failed\n");
        printf("Attempting to bind to SERVER_PORT_2...\n");

        // Attempt to bind serv_fd to SERVER_PORT_2
        serv_addr_2.sin_family = AF_INET;
        serv_addr_2.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr_2.sin_port = htons(SERVER_PORT_2);
        if (bind(serv_fd, (struct sockaddr *)&serv_addr_2, sizeof(serv_addr_2)) < 0) {
            printf("Error: bind SERVER_PORT_2 failed. Cannot start the server.\n");
            close(serv_fd);
            return 1;
        } else {
            printf("Successfully bound to SERVER_PORT_2.\n");
        }
    } else {
        printf("Server bound to SERVER_PORT_1.\n");
    }

    // listen for new connections
    if (listen(serv_fd, 5) < 0) {
        printf("Error: listen failed\n");
        close(serv_fd);
        return 1;
    }

    // Server loop
    while (1) {
        int *client_socket = malloc(sizeof(int));
        if (client_socket == NULL) {
            printf("Error: could not allocate memory for client socket\n");
            continue;
        }

        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        // accept new connection
        *client_socket = accept(serv_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (*client_socket < 0) {
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