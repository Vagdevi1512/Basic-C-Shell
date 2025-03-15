#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "processes.h"
#include "pipes.h"
#include "input.h"
#include "fab.h"

#define MAX_COMMANDS 10
#define MAX_ARGS 100

#define RED_TEXT "\033[31m"
#define RESET_COLOR "\033[0m"

// Function to parse a command into arguments preserving quotes
int parse_command(char *command, char *argv[])
{
    int argc = 0;
    char *current = command;

    while (*current != '\0')
    {
        // Skip leading spaces
        while (*current == ' ')
        {
            current++;
        }

        // If we encounter a quote, preserve the quoted string as a single argument
        if (*current == '\'')
        {
            char quote = *current;
            current++;
            argv[argc++] = current; // Start of the quoted argument
            while (*current != quote && *current != '\0')
            {
                current++;
            }
            if (*current == quote)
            {
                *current = '\0'; // Null-terminate the quoted argument
                current++;
            }
        }
        else
        {
            // Regular space-separated arguments
            argv[argc++] = current;
            while (*current != ' ' && *current != '\0')
            {
                current++;
            }
            if (*current == ' ')
            {
                *current = '\0'; // Null-terminate the argument
                current++;
            }
        }
    }

    argv[argc] = NULL; // Null-terminate the argument list
    return argc;
}

void execute_pipeline(char *pipeline)
{
    char *commands[MAX_COMMANDS];
    int command_count = 0;
    char *token = strtok(pipeline, "|");

    // Tokenize the input pipeline by "|"
    while (token != NULL && command_count < MAX_COMMANDS)
    {
        commands[command_count++] = token;
        token = strtok(NULL, "|");
    }

    if (command_count == 0)
    {
        // fprintf(stderr, "Invalid use of pipe\n");
        exit(EXIT_FAILURE);
    }

    int fd[2], input_fd = -1;

    for (int i = 0; i < command_count; i++)
    {
        pipe(fd); // Create a new pipe

        if (fork() == 0)
        { // Child process
            if (input_fd != -1)
            { // If there's an input pipe, redirect stdin
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            if (i < command_count - 1)
            { // If not the last command, redirect stdout to the pipe
                dup2(fd[1], STDOUT_FILENO);
            }

            close(fd[0]);
            close(fd[1]);

            // Prepare arguments for execvp
            char *argv[MAX_ARGS];
            parse_command(commands[i], argv);

            execvp(argv[0], argv);
            // perror("execvp");
            exit(EXIT_FAILURE);
        }
        else
        {               // Parent process
            wait(NULL); // Wait for the child process to finish
            close(fd[1]);
            if (input_fd != -1)
            {
                close(input_fd);
            }
            input_fd = fd[0]; // Save the input_fd for the next command
        }
    }
}

void pipes(const char *command) {
    char *pipeline = strdup(command);
    pipeline[strcspn(pipeline, "\n")] = '\0';
    execute_pipeline(pipeline);
    free(pipeline);
}


void fg_command(int pid)
{
    stopped_process_t *prev = NULL, *current = stopped_process_head;

    // Find the stopped process with the given PID
    while (current != NULL)
    {
        if (current->pid == pid)
        {
            break;
        }
        prev = current;
        current = current->next;
    }

    if (current == NULL)
    {
        // PID not found in stopped processes
        // printf("No such process found\n");
        printf(RED_TEXT "No such process found\n" RESET_COLOR);
        return;
    }

    // Remove the process from the stopped list
    if (prev == NULL)
    {
        stopped_process_head = current->next;
    }
    else
    {
        prev->next = current->next;
    }

    // Add the process to the foreground list
    // add_fg_process(current->pid, current->command);
    // printf("current->command = %s\n", current->command);
    dummy = current->pid;
    if(strchr(current->command, '&') != 0)
    {
        char *cmd=current->command;
        *strchr(cmd, '&') = '\0';
        execute_commands(current->command,1);
    }
    else
    execute_commands(current->command,0);
    free(current); 
}

// Function to move a stopped process to background and resume it
void bg_command(int pid)
{
    stopped_process_t *prev = NULL, *current = stopped_process_head;

    // Find the stopped process with the given PID
    while (current != NULL)
    {
        if (current->pid == pid)
        {
            break;
        }
        prev = current;
        current = current->next;
    }

    if (current == NULL)
    {
        // PID not found in stopped processes
        // printf("No such process found\n");
        printf(RED_TEXT "No such process found\n" RESET_COLOR);
        return;
    }

    // Remove the process from the stopped list
    if (prev == NULL)
    {
        stopped_process_head = current->next;
    }
    else
    {
        prev->next = current->next;
    }
    dummy = current->pid;
    // printf("%s",current->command);
    execute_commands(current->command,1);
    free(current);
}