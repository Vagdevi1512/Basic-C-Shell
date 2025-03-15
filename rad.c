#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>
#include <ctype.h>
#include "reveal.h"
#include "list.h"
#include "hop.h"
#include "log.h"
#include "seek.h"
#include "proclore.h"
#include "rad.h"
#include "fab.h"

#define MAX_COMMANDS 10
#define MAX_ARGS 100
#define MAX_PATH_LENGTH 1024

// Function to parse a command into arguments while handling quotes
int parse(char *command, char *argv[])
{
    int argc = 0;
    char *current = command;
    char *start;

    while (*current != '\0')
    {
        // Skip leading spaces
        while (*current == ' ')
        {
            current++;
        }

        if (*current == '\0')
            break; // End of command

        // If we encounter a quote, preserve the quoted string as a single argument
        if (*current == '\'' || *current == '\"')
        {
            char quote = *current;
            start = ++current; // Start of the quoted argument
            while (*current != quote && *current != '\0')
            {
                current++;
            }
            if (*current == quote)
            {
                *current = '\0'; // Null-terminate the quoted argument
                current++;
            }
            argv[argc++] = start;
        }
        else
        {
            // Regular space-separated arguments
            start = current;
            while (*current != ' ' && *current != '\0')
            {
                current++;
            }
            if (*current == ' ')
            {
                *current = '\0'; // Null-terminate the argument
                current++;
            }
            argv[argc++] = start;
        }
    }

    argv[argc] = NULL; // Null-terminate the argument list
    return argc;
}

// Function to handle input and output redirection
int handle_redirection(char *cmd, int *input_fd, int *output_fd)
{
    *input_fd = -1;
    *output_fd = -1;

    // Check for input redirection ("<")
    char *input_pos = strstr(cmd, "<");

    // Check for output redirection ("<<" or ">>")
    char *output_pos_append = strstr(cmd, ">>");
    char *output_pos = strstr(cmd, ">");

    if (input_pos)
    {
        *input_pos = '\0';                                 // Split the command at "<"
        char *input_file = strtok(input_pos + 1, " \t\n"); // Get the input file
        if (input_file)
        {
            *input_fd = open(input_file, O_RDONLY);
            if (*input_fd < 0)
            {
                fprintf(stderr, "Error: No such input file: %s\n", input_file);
                return -1;
            }
        }
    }

    if (output_pos_append)
    {
        *output_pos_append = '\0';                                  // Split the command at ">>"
        char *output_file = strtok(output_pos_append + 2, " \t\n"); // Get the output file
        if (output_file)
        {
            *output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (*output_fd < 0)
            {
                fprintf(stderr, "Error: Cannot open file for appending: %s\n", output_file);
                return -1;
            }
        }
    }
    else if (output_pos)
    {
        *output_pos = '\0';                                  // Split the command at ">"
        char *output_file = strtok(output_pos + 1, " \t\n"); // Get the output file
        if (output_file)
        {
            *output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (*output_fd < 0)
            {
                fprintf(stderr, "Error: Cannot open file for writing: %s\n", output_file);
                return -1;
            }
        }
    }

    return 0;
}

void execute(char *command, int is_background)
{
    int length = strlen(command);

    // Check for pipe at the end of the pipeline
    if (command[length - 1] == '|')
    {
        fprintf(stderr, "Invalid use of pipes\n");
        return;
    }

    // Check for consecutive pipes (i.e., || with no command in between)
    for (int i = 0; i < length - 1; i++)
    {
        if (command[i] == '|' && command[i + 1] == '|')
        {
            fprintf(stderr, "Invalid use of pipes\n");
            return;
        }
    }

    char *commands[MAX_COMMANDS];
    int command_count = 0;
    char *token = strtok(command, "|");

    // Tokenize the input pipeline by "|"
    while (token != NULL && command_count < MAX_COMMANDS)
    {
        while (*token == ' ')
            token++;
        if (*token == '\0')
        {
            fprintf(stderr, "Invalid use of pipes: Missing command between pipes\n");
            return;
        }

        commands[command_count++] = token;
        token = strtok(NULL, "|");
    }

    int fd[2], input_fd = -1, output_fd = -1;
    int prev_fd = -1; // For chaining pipes

    for (int i = 0; i < command_count; i++)
    {
        // Handle redirection for the current command
        if (handle_redirection(commands[i], &input_fd, &output_fd) < 0)
        {
            return; // If an error occurs, return and do not proceed
        }

        if (i < command_count - 1)
        {
            pipe(fd); // Create a new pipe for intermediate commands
        }

        pid_t pid = fork();
        if (pid == 0)
        { // Child process
            // Input redirection for the command
            if (input_fd != -1)
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            // Output redirection for the command
            if (output_fd != -1)
            {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }
            else if (i < command_count - 1)
            { // Redirect stdout to pipe for intermediate commands
                dup2(fd[1], STDOUT_FILENO);
            }

            // For intermediate commands, read from the previous command's output
            if (i > 0)
            {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            // Close all unused file descriptors
            close(fd[0]); // Close the read-end of the pipe
            if (i < command_count - 1)
                close(fd[1]); // Close the write-end of the pipe

            // Prepare arguments for execvp
            char *argv[MAX_ARGS];
            parse(commands[i], argv);
            if (strncmp(argv[0], "reveal", 6) == 0)
            {
                int EXTRA_SPACE = 20;
                int start_index = 1;
                size_t total_length = 0;
                for (int i = start_index; i < MAX_ARGS && argv[i] != NULL; i++)
                {
                    total_length += strlen(argv[i]);
                }
                total_length += (MAX_ARGS - start_index - 1) * 2; // Adding space for "  " between arguments
                total_length += EXTRA_SPACE;                      // Extra space for safety
                total_length++;                                   // For null terminator

                // Allocate and initialize memory
                char *opts = (char *)calloc(total_length, sizeof(char));
                if (opts == NULL)
                {
                    perror("Failed to allocate memory");
                    exit(EXIT_FAILURE);
                }

                // Concatenate arguments
                for (int i = start_index; i < MAX_ARGS && argv[i] != NULL; i++)
                {
                    if (i > start_index)
                    {
                        strcat(opts, "  "); // Add space between arguments
                    }
                    strcat(opts, argv[i]);
                }

                int show_hidden = 0;
                int detailed_info = 0;

                while (*opts)
                {
                    if (strncmp(opts, "-", 1) == 0)
                    {
                        opts += 1;
                        if (strncmp(opts, "-", 1) == 0)
                        {
                            // printf("Error: Invalid option format.\n");
                            return;
                        }
                        while (strncmp(opts, "a", 1) == 0 || strncmp(opts, "l", 1) == 0)
                        {
                            if (strncmp(opts, "a", 1) == 0)
                            {
                                show_hidden = 1;
                                opts += 1;
                            }
                            else if (strncmp(opts, "l", 1) == 0)
                            {
                                detailed_info = 1;
                                opts += 1;
                            }
                        }
                        while (strncmp(opts, " ", 1) == 0)
                        {
                            opts++;
                        }
                    }
                    else if (strncmp(opts, " ", 1) == 0)
                    {
                        opts += 1;
                    }
                    else if (strncmp(opts, "/", 1) == 0 || strncmp(opts, "./", 2) == 0 || strncmp(opts, "h", 1) == 0)
                    {
                        break;
                    }
                    else
                    {
                        opts += 1;
                    }
                }
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd)) == NULL)
                {
                    perror("getcwd");
                    exit(EXIT_FAILURE);
                }
                if (strlen(opts) == 0)
                {
                    opts = cwd;
                }
                list_files(opts, show_hidden, detailed_info);
            }
            else if (strncmp(argv[0], "hop", 3) == 0)
            {
                if (argv[2] == NULL)
                {
                    hop(argv[1]);
                }
                else
                {
                    // Allocate memory for the new string
                    char vi[MAX_PATH_LENGTH];
                    snprintf(vi, sizeof(vi), "%s %s %s", argv[1], " ", argv[2]);
                    const char *vii = vi;
                    hop(vii);
                }
            }
            else if (strncmp(argv[0], "log", 3) == 0 && argv[1] != NULL)
            {
                if (strcmp(argv[1], "execute") == 0)
                {
                    char *endptr;
                    long arg_val = strtol(argv[2], &endptr, 10);
                    if (*endptr != '\0' || arg_val > INT_MAX || arg_val < INT_MIN)
                    {
                        fprintf(stderr, "Error: Invalid number provided: %s\n", argv[2]);
                        return;
                    }
                    int number = (int)arg_val;
                    log_execute(number);
                }
            }
            else if (strncmp(argv[0], "log", 3) == 0)
            {
                log_commands();
            }
            else if (strcmp(argv[0], "proclore") == 0 && argv[1] == NULL)
            {
                pid_t pid = getpid(); // No PID provided, use the current shell's PID
                proclore(pid);
            }
            else if (strncmp(argv[0], "proclore", 8) == 0)
            {
                pid_t pid;

                if (strcmp(argv[1], ">") != 0 && strcmp(argv[1], "<") != 0 && strcmp(argv[1], "<<") != 0)
                {
                    pid = atoi(argv[1]);
                }
                else
                {
                    pid = getpid(); // No valid PID provided, use current shell's PID
                }
                proclore(pid);
            }
            else if (strncmp(argv[0], "seek", 4) == 0)
            {
                int EXTRA_SPACE = 20;
                int start_index = 1;
                size_t total_length = 0;
                for (int i = start_index; i < MAX_ARGS && argv[i] != NULL; i++)
                {
                    total_length += strlen(argv[i]);
                }
                total_length += (MAX_ARGS - start_index - 1) * 2; // Adding space for "  " between arguments
                total_length += EXTRA_SPACE;                      // Extra space for safety
                total_length++;                                   // For null terminator

                // Allocate and initialize memory
                char *opts = (char *)calloc(total_length, sizeof(char));
                if (opts == NULL)
                {
                    perror("Failed to allocate memory");
                    exit(EXIT_FAILURE);
                }

                // Concatenate arguments
                for (int i = start_index; i < MAX_ARGS && argv[i] != NULL; i++)
                {
                    if (i > start_index)
                    {
                        strcat(opts, "  "); // Add space between arguments
                    }
                    strcat(opts, argv[i]);
                }
                seek_files(opts);
                free(opts); // Free allocated memory
            }
            execvp(argv[0], argv);
            exit(EXIT_FAILURE);
        }
        else if (pid < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else
        {
            if (is_background)
            {
                // If running in the background, do not wait for the child process
                printf("Started background process with PID %d\n", pid);
                if (i < command_count - 1)
                {
                    close(fd[1]); // Close the write-end in the parent
                    if (prev_fd != -1)
                    {
                        close(prev_fd); // Close the previous read-end
                    }
                    prev_fd = fd[0]; // Set the new read-end for the next command
                }
                // execute_commands(command,1);
            }
            else
            {
                // Wait for the child process to finish if not running in the background
                waitpid(pid, NULL, 0);
                if (i < command_count - 1)
                {
                    close(fd[1]); // Close the write-end in the parent
                    if (prev_fd != -1)
                    {
                        close(prev_fd); // Close the previous read-end
                    }
                    prev_fd = fd[0]; // Set the new read-end for the next command
                }
            }
        }
    }

    // Clean up
    if (input_fd != -1)
        close(input_fd);
    if (output_fd != -1)
        close(output_fd);
}

void tokenize_and_execute(char *input)
{
    char *commands[MAX_COMMANDS];
    int command_count = 0;
    char *cmd = strtok(input, ";");

    // Tokenize the input by ";"
    while (cmd != NULL && command_count < MAX_COMMANDS)
    {
        commands[command_count] = cmd;
        command_count++;
        cmd = strtok(NULL, ";");
    }

    // Execute each command
    for (int i = 0; i < command_count; i++)
    {
        char *command = commands[i];
        while (*command == ' ')
            command++; // Trim leading spaces

        char *end = command + strlen(command) - 1;
        while (end > command && *end == ' ')
            end--;         // Trim trailing spaces
        *(end + 1) = '\0'; // Null-terminate

        if (*command != '\0')
        {
            char *ampersand_pos = NULL;

            // Find the last occurrence of '&'
            for (char *p = command; *p != '\0'; p++)
            {
                if (*p == '&')
                {
                    ampersand_pos = p;
                }
            }

            // If we found an '&'
            if (ampersand_pos != NULL)
            {
                // First, execute the part before '&' in the background
                *ampersand_pos = '\0'; // Split the command at '&'

                // Trim spaces before executing
                char *first_part = command;
                while (*first_part == ' ')
                    first_part++; // Trim leading spaces
                end = first_part + strlen(first_part) - 1;
                while (end > first_part && *end == ' ')
                    end--;         // Trim trailing spaces
                *(end + 1) = '\0'; // Null-terminate

                if (*first_part != '\0')
                {
                    execute(first_part, 1); // Run in background
                }

                // Execute the part after '&' in the foreground
                char *second_part = ampersand_pos + 1; // Move to the part after '&'

                // Trim spaces before executing
                while (*second_part == ' ')
                    second_part++; // Trim leading spaces
                end = second_part + strlen(second_part) - 1;
                while (end > second_part && *end == ' ')
                    end--;         // Trim trailing spaces
                *(end + 1) = '\0'; // Null-terminate

                if (*second_part != '\0')
                {
                    execute(second_part, 0); // Run in foreground
                }
            }
            else
            {
                // No '&' found, run normally in foreground
                execute(command, 0);
            }
        }
    }
}

void rad(const char *cmd)
{
    char *copy_cmd = strdup(cmd);
    copy_cmd[strcspn(copy_cmd, "\n")] = '\0';
    tokenize_and_execute(copy_cmd);
    free(copy_cmd); // Don't forget to free the duplicated command string
}
