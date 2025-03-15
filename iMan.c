#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#define RED_TEXT "\033[31m"
#define RESET_COLOR "\033[0m"

#define BUFFER_SIZE 8192
#define HTTP_PORT 80
#define REQUEST_SIZE 1024 // Increased buffer size for HTTP request

// Helper function to URL-encode the command
void url_encode(char *dest, const char *src)
{
    while (*src)
    {
        if (isalnum((unsigned char)*src) || *src == '-' || *src == '_' || *src == '.' || *src == '~')
        {
            *dest++ = *src;
        }
        else
        {
            *dest++ = '%';
            *dest++ = "0123456789ABCDEF"[(*src >> 4) & 0xF];
            *dest++ = "0123456789ABCDEF"[*src & 0xF];
        }
        src++;
    }
    *dest = '\0';
}

void fetch_man_page(const char *command)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    char request[REQUEST_SIZE];
    char encoded_command[512];
    int line_count = 0;
    int found_error = 0; // Flag to check for error messages

    // URL-encode the command
    url_encode(encoded_command, command);

    // Prepare the HTTP GET request with /man1/ prefix
    snprintf(request, sizeof(request),
             "GET /man1/%s HTTP/1.1\r\n"
             "Host: man.he.net\r\n"
             "Connection: close\r\n"
             "\r\n",
             encoded_command);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    // Get server IP address
    if ((server = gethostbyname("man.he.net")) == NULL)
    {
        fprintf(stderr, "Error: No such host\n");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(HTTP_PORT);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error connecting");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send the GET request
    if (write(sockfd, request, strlen(request)) < 0)
    {
        perror("Error writing to socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_received;
    int inside_tag = 0;
    while ((bytes_received = read(sockfd, buffer, sizeof(buffer) - 1)) > 0)
    {
        for (int i = 0; i < bytes_received; i++)
        {
            if (buffer[i] == '\n')
            {
                line_count++; // Increment line count when a new line is detected
            }

            // Detect if the error page is present (based on common error messages)
            if (strstr(buffer, "404 Not Found") != NULL || strstr(buffer, "No matches for") != NULL)
            {
                found_error = 1; // Mark that the error was found
            }

            if (line_count >= 7) // Start printing after the first 7 lines if no error
            {
                if (buffer[i] == '<')
                {
                    inside_tag = 1;
                }
                else if (buffer[i] == '>')
                {
                    inside_tag = 0;
                }
                else if (!inside_tag)
                {
                    printf("%c", buffer[i]);
                }
            }
        }
    }

    if (bytes_received < 0)
    {
        perror("Error reading from socket");
    }

    close(sockfd);

    // If error found, print the error message
    if (found_error)
    {
        // printf("ERROR: No matches for '%s' command\n", command);
        printf(RED_TEXT "ERROR: No matches for '%s' command\n" RESET_COLOR, command);
    }
}
int iMan(char *arg)
{
    // Find the first space after "iMan "
    char *first_space = strchr(arg + 5, ' ');

    // If there's a space, terminate the string at that position (ignoring extra arguments)
    if (first_space != NULL)
    {
        *first_space = '\0'; // Terminate the string to keep only the first argument
    }

    // Allocate memory for the command name (ignoring the "iMan " part)
    char *command_name = malloc(strlen(arg) - 4 + 1); // +1 for the null terminator
    if (command_name == NULL)
    {
        perror("Error allocating memory");
        return EXIT_FAILURE;
    }

    // Copy the first argument into command_name
    strcpy(command_name, arg + 5); // Copy the part after "iMan "

    // Fetch the man page for the command_name
    fetch_man_page(command_name);

    // Free allocated memory
    // free(command_name);

    return EXIT_SUCCESS;
}
