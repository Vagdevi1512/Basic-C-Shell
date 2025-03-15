#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include "redirection.h"

#define MAX_ARGS 100

void apply_redirections(int *input_fd, int *output_fd)
{
    if (*input_fd != -1)
    {
        if (dup2(*input_fd, STDIN_FILENO) < 0)
        {
            perror("dup2 for input");
            exit(EXIT_FAILURE);
        }
        close(*input_fd); // Close the file descriptor after duplicating
    }
    if (*output_fd != -1)
    {
        if (dup2(*output_fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 for output");
            exit(EXIT_FAILURE);
        }
        close(*output_fd); // Close the file descriptor after duplicating
    }
}

void handle_redirections(char **args)
{
    int input_fd = -1, output_fd = -1;
    // int pipefd[2];
    pid_t pid;
    int i = 0;
    int command_start = 0;
    int pipe_in = -1;  // To store the read end of the current pipe

    while (args[i] != NULL)
    {
        // if (strcmp(args[i], "|") == 0)
        // {
        //     args[i] = NULL; // End of arguments for the current command

        //     if (pipe(pipefd) < 0)
        //     {
        //         perror("pipe");
        //         exit(EXIT_FAILURE);
        //     }

        //     if ((pid = fork()) == 0)
        //     {
        //         if (pipe_in != -1) {
        //             dup2(pipe_in, STDIN_FILENO);
        //             close(pipe_in);
        //         }
        //         dup2(pipefd[1], STDOUT_FILENO);
        //         close(pipefd[0]);
        //         execvp(args[command_start], &args[command_start]);
        //         // perror("exec");
        //         exit(EXIT_FAILURE);
        //     }
        //     else
        //     {
        //         wait(NULL); // Wait for the child process to finish
        //         close(pipefd[1]); // Close the write end of the pipe
        //         pipe_in = pipefd[0]; // Set up input for the next command
        //         command_start = i + 1; // Move to the next command
        //     }
        // }
        if (args[i] != NULL && strcmp(args[i], "<") == 0)
        {
            if (i + 1 < MAX_ARGS)
            {
                input_fd = open(args[i + 1], O_RDONLY);
                if (input_fd < 0)
                {
                    fprintf(stderr, "No such input file found: %s\n", args[i + 1]);
                    input_fd = -1; // Reset input_fd to -1 to avoid incorrect usage
                    return;
                }
                args[i] = NULL;     // End of arguments for the command
                args[i + 1] = NULL; // Remove redirection part from arguments
            }
            if(strcmp(args[i+2], ">")==0) {
                i++;
                if (i + 2 < MAX_ARGS)
                {
                    output_fd = open(args[i + 2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (output_fd < 0)
                    {
                        fprintf(stderr, "Error opening file for writing: %s\n", args[i + 2]);
                        output_fd = -1; // Reset output_fd to -1 to avoid incorrect usage
                        return;
                    }
                    args[i+1] = NULL;     // End of arguments for the command
                    args[i + 2] = NULL; // Remove redirection part from arguments
                }
            }
        }
        else if (strcmp(args[i], ">") == 0)
        {
            if (i + 1 < MAX_ARGS)
            {
                output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_fd < 0)
                {
                    fprintf(stderr, "Error opening file for writing: %s\n", args[i + 1]);
                    output_fd = -1; // Reset output_fd to -1 to avoid incorrect usage
                    return;
                }
                args[i] = NULL;     // End of arguments for the command
                args[i + 1] = NULL; // Remove redirection part from arguments
            }
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            if (i + 1 < MAX_ARGS)
            {
                output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (output_fd < 0)
                {
                    fprintf(stderr, "Error opening file for appending: %s\n", args[i + 1]);
                    output_fd = -1; // Reset output_fd to -1 to avoid incorrect usage
                    return;
                }
                args[i] = NULL;     // End of arguments for the command
                args[i + 1] = NULL; // Remove redirection part from arguments
            }
        }
        i++;
    }

    // Execute the final command
    if ((pid = fork()) == 0)
    {
        if (pipe_in != -1) {
            dup2(pipe_in, STDIN_FILENO);
            close(pipe_in);
        }
        apply_redirections(&input_fd, &output_fd);
        execvp(args[command_start], &args[command_start]);
        // perror("exec");
        exit(EXIT_FAILURE);
    }
    wait(NULL); // Wait for the final command to finish
}

void redirection(const char *input)
{
    char *input_copy = strdup(input);
    if (input_copy == NULL)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    char *args[MAX_ARGS];
    char *token = strtok(input_copy, " ");
    int arg_count = 0;

    // Parse input into arguments
    while (token != NULL)
    {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    // Handle redirection and pipes
    handle_redirections(args);

    free(input_copy);
}
