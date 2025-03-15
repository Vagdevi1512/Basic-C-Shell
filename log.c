#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "input.h"
#include <ctype.h>
#include <sys/wait.h>
#include <stddef.h>

#define MAX_COMMANDS 15
#define MAX_COMMAND_LEN 256

#define COLOR_RESET    "\033[0m"
#define COLOR_RED      "\033[31m"

static char log_file_path[1024] = {0};

// Initialize the log file path
void initialize_log_file(const char *directory)
{
    snprintf(log_file_path, sizeof(log_file_path), "%s/commands.txt", directory);
}
// Function to display the last 15 commands
void log_commands()
{
    FILE *file = fopen(log_file_path, "r");
    if (!file)
    {
        fprintf(stderr, COLOR_RED "ERROR: Error opening log file.\n" COLOR_RESET);
        return;
    }

    char commands[MAX_COMMANDS][MAX_COMMAND_LEN];
    int count = 0;

    // Read commands into array and remove the trailing newline
    while (fgets(commands[count], MAX_COMMAND_LEN, file) != NULL && count < MAX_COMMANDS)
    {
        commands[count][strcspn(commands[count], "\n")] = '\0'; // Remove newline
        count++;
    }

    fclose(file);

    // Handle case where no commands were found
    if (count == 0)
    {
        printf("No commands found in the log.\n");
        return;
    }

    // Print commands in reverse order (most recent first)
    for (int i = count - 1; i >= 0; i--)
    {
        printf("%d: %s\n", count - i, commands[i]);
    }
}


// Function to clear the file
void log_purge()
{
    FILE *file = fopen(log_file_path, "w");
    if (!file)
    {
        fprintf(stderr, COLOR_RED "ERROR: Error opening file.\n" COLOR_RESET);
        return;
    }

    // Closing the file immediately clears it
    fclose(file);
    printf("Command log cleared.\n");
}


void log_execute(int index)
{
    FILE *file = fopen(log_file_path, "r");
    if (!file)
    {
        fprintf(stderr, COLOR_RED "ERROR: Error opening file.\n" COLOR_RESET);
        return;
    }

    char command[MAX_COMMAND_LEN];
    int count = 0;

    // Calculate the total number of commands
    while (fgets(command, MAX_COMMAND_LEN, file) != NULL)
    {
        count++;
    }
    rewind(file);

    // Validate index
    if (index < 1 || index > count)
    {
        fclose(file);
        fprintf(stderr, COLOR_RED "ERROR: Invalid index.\n" COLOR_RESET);
        return;
    }

    // Find the command at the given index
    int current_index = 0;
    while (fgets(command, MAX_COMMAND_LEN, file) != NULL)
    {
        if (++current_index == count - index + 1)
        {
            // Ensure the command is properly formatted
            if (command[strlen(command) - 1] == '\n')
            {
                command[strlen(command) - 1] = '\0'; // Remove trailing newline
            }

            printf("Executing: %s\n", command);
            handle_input(command, 0, strlen(command));  
           log_new_command(command);
            fclose(file);
            return;
        }
    }

    fclose(file);
}


// Function to log a new command
void log_new_command(const char *new_command)
{
    // Attempt to open the log file in read mode first
    FILE *file = fopen(log_file_path, "r");
    char commands[MAX_COMMANDS][MAX_COMMAND_LEN];
    int count = 0;

    // Read existing commands if the file exists and is readable
    if (file)
    {
        // Read existing commands into array
        while (fgets(commands[count], MAX_COMMAND_LEN, file) != NULL && count < MAX_COMMANDS)
        {
            // Remove trailing newline from each command
            commands[count][strcspn(commands[count], "\n")] = '\0';
            count++;
        }
        fclose(file);
    }

    // Check if the new command is a duplicate of the last logged command
    if (count > 0 && strcmp(commands[count - 1], new_command) == 0)
    {
        // Do not log the new command if it's identical to the previous command
        return;
    }

    // If the command list is full, shift the commands to make space for the new one
    if (count == MAX_COMMANDS)
    {
        count--;
    }

    // Add the new command at the end
    snprintf(commands[count], MAX_COMMAND_LEN, "%s", new_command);

    // Reopen the file in write mode to save the updated log
    file = fopen(log_file_path, "w");
    if (!file)
    {
        fprintf(stderr, COLOR_RED "ERROR: Error opening log file for writing.\n" COLOR_RESET);
        return;
    }

    // Write the updated list of commands to the file
    for (int i = 0; i <= count; i++)
    {
        fprintf(file, "%s\n", commands[i]);  // Ensure each command ends with a newline
    }

    fclose(file);
}

