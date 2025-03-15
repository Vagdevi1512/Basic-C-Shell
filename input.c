#include "input.h"
#include "hop.h"
#include "reveal.h"
#include "list.h"
#include "proclore.h"
#include "log.h"
#include "seek.h"
#include "fab.h"
#include "redirection.h"
#include "pipes.h"
#include "rad.h"
#include "iMan.h"
#include "fgbg.h"
#include "ping.h"
#include "pipes.h"
#include "neonate.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_CMD_LENGTH 1024

// Color codes
#define RED_TEXT "\033[31m"
#define RESET_COLOR "\033[0m"

int string_to_pid(const char *str)
{
    return atoi(str); // Convert string to integer
}

// Function to trim leading and trailing spaces from a string
char *trim(char *str)
{
    // Trim leading spaces
    while (isspace((unsigned char)*str))
        str++;

    // If all spaces
    if (*str == '\0')
        return str;

    // Trim trailing spaces
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Null-terminate the trimmed string
    *(end + 1) = '\0';

    return str;
}

void execute_command(const char *cmd)
{
    // printf("%s", cmd);
    // printf("Hell");
    // if (strncmp(cmd, "cat", 3) == 0)
    // {
    //     redirection(cmd);
    // }
    if (strchr(cmd, '>') != NULL || strchr(cmd, '<') != NULL || strchr(cmd, '|') != NULL || strncmp(cmd,"cat", 3) == 0)
    {
        // printf("HI\n");
        rad(cmd);
    }
    else if (strchr(cmd, '&') != NULL || strchr(cmd, ';') != NULL || strncmp(cmd, "sleep", 5) == 0)
    {
        // printf("sleep");
        fab(cmd);
        return;
    }
    else if (strncmp(cmd, "fg", 2) == 0)
    {
        char *command = strdup(cmd); // Duplicate the command string for modification
        if (command == NULL)
        {
            perror("strdup failed");
            return;
        }

        // Extract the command type (e.g., "fg" or "bg")
        char *command_type = strtok(command, " \t");
        if (command_type == NULL)
        {
            printf("Failed to parse command type.\n");
            free(command);
            return;
        }

        char *pid_str = strtok(NULL, " \t"); // Extract the PID part
        if (pid_str != NULL)
        {
            trim(pid_str); // Remove leading and trailing spaces
        }
        else
        {
            printf("No PID provided.\n");
            free(command);
            return;
        }

        int pid = atoi(pid_str); // Convert PID to integer
        fg_command(pid);
        if (pid <= 0)
        {
            printf("Invalid PID.\n");
            free(command);
            return;
        }
        // fg(pid);
    }
    else if (strncmp(cmd, "bg", 2) == 0)
    {
        if (cmd == NULL)
        {
            printf("Command is NULL.\n");
            return;
        }

        // Create a mutable copy of cmd
        char *command = strdup(cmd); // Duplicate the command string for modification
        if (command == NULL)
        {
            perror("strdup failed");
            return;
        }

        // Extract the command type (e.g., "fg" or "bg")
        char *command_type = strtok(command, " \t");
        if (command_type == NULL)
        {
            printf("Failed to parse command type.\n");
            free(command);
            return;
        }

        char *pid_str = strtok(NULL, " \t"); // Extract the PID part
        if (pid_str != NULL)
        {
            trim(pid_str); // Remove leading and trailing spaces
        }
        else
        {
            printf("No PID provided.\n");
            free(command);
            return;
        }

        int pid = atoi(pid_str); // Convert PID to integer
        bg_command(pid);
        if (pid <= 0)
        {
            printf("Invalid PID.\n");
            free(command);
            return;
        }
        // bg(pid);
    }
    else if (strncmp(cmd, "ping ", 5) == 0)
    {
        // Extract the part of the command after "ping "
        char *command = strdup(cmd + 5);
        if (command == NULL)
        {
            fprintf(stderr, "Memory allocation error\n");
            exit(EXIT_FAILURE);
        }

        // Extract PID and signal number from the command
        char *pid_str = strtok(command, " ");
        char *signal_str = strtok(NULL, " ");

        if (pid_str == NULL || signal_str == NULL)
        {
            printf("Usage: ping <pid> <signal_number>\n");
            free(command);
            return;
        }

        // Convert strings to integers
        int pid = atoi(pid_str);
        int signal_number = atoi(signal_str);

        // Call the ping_command function
        ping_command(pid, signal_number);

        free(command);
    }
    else if (strncmp(cmd, "neonate ", 7) == 0)
    {
        // Extract the part of the command after "ping "
        char *command = strdup(cmd + 8);
        if (command == NULL)
        {
            fprintf(stderr, "Memory allocation error\n");
            exit(EXIT_FAILURE);
        }

        // Extract PID and signal number from the command
        char *some = strtok(command, " ");
        char *arg_sec = strtok(NULL, " ");

        if (some == NULL || arg_sec == NULL)
        {
            printf("Usage: neonate -n <srg_sec>\n");
            free(command);
            return;
        }

        // Convert strings to integers
        int arg = atoi(arg_sec);
        neonate(arg);
        free(command);
    }
    else if (strncmp(cmd, "hop ", 4) == 0)
    {
        // Extract the path to hop to
        const char *path = cmd + 4;
        hop(path);
    }
    else if (strncmp(cmd, "proclore", 8) == 0)
    {
        pid_t pid;

        if (strlen(cmd) > 8)
        {
            // Extract the PID from the command string (cmd + 9 points to the first character after "proclore ")
            pid = atoi(cmd + 9);
        }
        else
        {
            // No PID provided, use the current shell's PID
            pid = getpid();
        }

        // Call the proclore function with the determined PID
        proclore(pid);
    }
    else if (strncmp(cmd, "seek ", 5) == 0)
    {
        // Extract the parameters for seek
        const char *params = cmd + 5;
        seek_files(params);
    }
    else if (strcmp(cmd, "reveal ~") == 0)
    {
        // Extract the path for revealz
        const char *path = "~";
        reveal(path);
    }
    else if (strcmp(cmd, "reveal -") == 0)
    {
        // Extract the path for reveal
        const char *path = "-";
        reveal(path);
    }
    else if (strcmp(cmd, "reveal") == 0)
    {
        const char *path = ".";
        reveal(path);
    }
    else if (strcmp(cmd, "reveal .") == 0)
    {
        const char *path = ".";
        reveal(path);
    }
    else if (strcmp(cmd, "reveal ..") == 0)
    {
        const char *path = "..";
        reveal(path);
    }
    else if (strncmp(cmd, "reveal ", 7) == 0)
    {
        // Extract options for list
        int show_hidden = 0;
        int detailed_info = 0;

        char *opts = (char *)cmd + 7; // Skip "reveal"
        while (*opts)
        {
            if (strncmp(opts, "-", 1) == 0)
            {
                opts += 1;
                if (strncmp(opts, "-", 1) == 0)
                {
                    printf(RED_TEXT "Error: Invalid option format.\n" RESET_COLOR);
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
            else if (strncmp(opts, "/", 1) == 0)
            {
                break;
            }
            else if (strncmp(opts, "./", 2) == 0)
            {
                break;
            }
            else if (strncmp(opts, "h", 1) == 0)
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
    else if (strncmp(cmd, "sleep", 5) == 0 || strncmp(cmd, "echo", 4) == 0 || strncmp(cmd, "gedit", 5) == 0 || strncmp(cmd, "vim", 3) == 0)
    {
        // printf("Hello");
        fab(cmd);
        return;
    }
    else if(strncmp(cmd, "log",3)==0 || strncmp(cmd, "activities",10)==0 || strncmp(cmd,"iMan",4)==0){}
    else
    {
        printf("Invalid command\n");
        return;
    }
}

void handle_input(const char *input, int t, int k)
{
    // Create a mutable copy of input
    char *input_copy = strdup(input);
    if (!input_copy)
    {
        fprintf(stderr, RED_TEXT "Error: strdup failed\n" RESET_COLOR);
        exit(EXIT_FAILURE);
    }

    // Tokenize the input based on new lines and semicolons
    char *command = strtok(input_copy, "\n");
    command = trim(command);
    // int k = strlen(command);
    // printf("log");

    // printf("%d\n", strlen(command));
    while (command != NULL && k > 0)
    {
        // Trim leading and trailing spaces
        command = trim(command);
        // printf("%s", command);
        // printf("%d\n", strlen(command));
        // printf("log");

        // if (strchr(command, '>') != NULL || strchr(command, '<') != NULL || strchr(command, '|') != NULL)
        // {
        //     // printf("HI\n");
        //     rad(command);
        // }

        if (*command && k > 0) // Check if command is not empty
        {
            // Check if the command is a special 'log' command
            if (strcmp(command, "log purge") == 0)
            {
                log_purge();
            }
            else if (strncmp(command, "log execute ", 12) == 0)
            {
                int index = atoi(command + 12); // Extract the index from the command
                if (index >= 0)
                {
                    log_execute(index); // Execute the command from the log
                }
                else
                {
                    fprintf(stderr, "Invalid index for log execute\n");
                }
            }
            else if (strcmp(command, "log") == 0 || strcmp(command, "log ") == 0)
            {
                log_commands();
            }
            else
            {
                if (t == 1)
                {
                    // printf("%s", command);
                    log_new_command(command);
                    k = 0;
                }
                execute_command(command); // Handle other commands
            }
        }

        // Get the next command
        command = strtok(NULL, "\n;");
    }

    free(input_copy);
}

