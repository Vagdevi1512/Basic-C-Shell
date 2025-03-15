#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include "fab.h"
#include "processes.h"
#include "pipes.h"
#include "input.h"
#include "time.h"

#define MAX_CMD_LEN 1024
#define MAX_TOKENS 128
#define MAX_JOBS 128

int dummy;
int var;

time_t now;
time_t end;

char *display_input;

// Color codes
#define RED_TEXT "\033[31m"
#define RESET_COLOR "\033[0m"

int background_job_count = 0;
pid_t background_jobs[MAX_JOBS];
char *input_copy;

// Updated SIGCHLD handler to reap completed processes and remove them from lists
void sigchld_handler(int signum)
{
    int status;
    pid_t pid;

    // Reap all child processes that have terminated
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    {
        // Check if the process is in the background list

        bg_process_t2 *bg_curr2 = bg_list_head2;

        while (bg_curr2 != NULL)
        {

            if (bg_curr2->pid == pid)
            {
                if (WIFEXITED(status))
                {
                    printf("Background job [%d] (%d) completed with exit status %d\n", background_job_count, pid, WEXITSTATUS(status));
                }
                else if (WIFSIGNALED(status))
                {
                    printf("Background job [%d] (%d) terminated by signal %d\n", background_job_count, pid, WTERMSIG(status));
                }
                remove_bg_process2(pid); // Remove from the background list
                fflush(stdout);
                break;
            }

            bg_curr2 = bg_curr2->next;
        }

        bg_process_t *bg_curr = bg_list_head;
        while (bg_curr != NULL)
        {
            if (strncmp(input_copy, "fg", 2) == 0 || strncmp(input_copy, "bg", 2) == 0)
            {

                if (bg_curr->pid == pid)
                {
                    if (WIFEXITED(status))
                    {
                        printf("Background job [%d] (%d) completed with exit status %d\n", background_job_count, dummy, WEXITSTATUS(status));
                    }
                    else if (WIFSIGNALED(status))
                    {
                        printf("Background job [%d] (%d) terminated by signal %d\n", background_job_count, dummy, WTERMSIG(status));
                    }
                    remove_bg_process(pid); // Remove from the background list
                    fflush(stdout);
                    break;
                }
            }
            else
            {
                if (bg_curr->pid == pid)
                {
                    if (WIFEXITED(status))
                    {
                        printf("Background job [%d] (%d) completed with exit status %d\n", background_job_count, pid, WEXITSTATUS(status));
                    }
                    else if (WIFSIGNALED(status))
                    {
                        printf("Background job [%d] (%d) terminated by signal %d\n", background_job_count, pid, WTERMSIG(status));
                    }
                    remove_bg_process(pid); // Remove from the background list
                    fflush(stdout);
                    break;
                }
            }
            bg_curr = bg_curr->next;
        }

        // If not found in background, check the foreground list
        fg_process_t *fg_curr = fg_list_head;
        while (fg_curr != NULL)
        {
            if (fg_curr->pid == pid)
            {
                // Print the completion status for foreground jobs
                if (WIFEXITED(status))
                {
                    // printf("Foreground job [%d] exited with status %d\n", pid, WEXITSTATUS(status));
                }
                else if (WIFSIGNALED(status))
                {
                    // printf("Foreground job [%d] terminated by signal %d\n", pid, WTERMSIG(status));
                }
                remove_fg_process(pid); // Remove from the foreground list
                fflush(stdout);
                break;
            }
            fg_curr = fg_curr->next;
        }
    }
}

// Function to trim leading and trailing spaces from a string
char *trim_spaces(char *str)
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

void execute_commands(char *command, int background)
{
    if (command == NULL)
    {
        return;
    }

    char *tokens[MAX_TOKENS];
    int i = 0;

    // Tokenize the command based on spaces
    tokens[i] = strtok(command, " \t");
    while (tokens[i] != NULL && i < MAX_TOKENS - 1)
    {
        i++;
        tokens[i] = strtok(NULL, " \t");
    }
    tokens[i] = NULL; // Null-terminate the tokens array

    if (tokens[0] == NULL)
    {
        return; // No command entered
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process
        if (background)
        {
            // Ignore SIGINT in background processes
            signal(SIGINT, SIG_IGN);
        }

        if (execvp(tokens[0], tokens) == -1)
        {
            // perror("execvp");
            exit(EXIT_FAILURE); // Exit if command execution fails
        }
    }
    else if (pid > 0)
    {
        // Parent process
        if (background)
        {
            if (var == 1)
            {
                char *input = strcat(input_copy, " & ");
                add_bg_process(pid, input);
            }
            else
                add_bg_process(pid, input_copy);
            if (strncmp(input_copy, "fg", 2) == 0 || strncmp(input_copy, "bg", 2) == 0)
            {
                printf("[%d] %d \n", background_job_count, dummy);
            }
            else
                printf("[%d] %d\n", background_job_count, pid);
            fflush(stdout);
        }
        else
        {
            if (var == 1)
            {
                char *input = strcat(input_copy, " & ");
                add_fg_process(pid, input);
            }
            else
                add_fg_process(pid, input_copy);

            // Foreground processes should stop on Ctrl-C or Ctrl-Z
            int status;
            while (1)
            {
                now = time(NULL);
                pid_t result = waitpid(pid, &status, WUNTRACED);
                end = time(NULL);
                if (result == -1)
                {
                    // perror("waitpid");
                    break;
                }

                if (WIFEXITED(status))
                {
                    break;
                }
                else if (WIFSIGNALED(status))
                {
                    break;
                }
                else if (WIFSTOPPED(status))
                {
                    break; // Exit the loop since the process is now stopped
                }
                else
                {
                    return;
                }
            }

            // Remove the foreground process after it stops or completes
            remove_fg_process(pid);
        }
    }
    else
    {
        // Fork failed
        fprintf(stderr, RED_TEXT "fork error: %s\n" RESET_COLOR, strerror(errno));
    }
}

void process_input(char *input)
{
    char *command;
    char *saveptr;
    int k = 0;
    if (input[strlen(input) - 1] == '&')
    {
        k = 1;
    }
    command = strtok_r(input, ";", &saveptr);
    while (command != NULL)
    {
        char *cmd = strtok(command, "&");
        char *commands[MAX_TOKENS];
        int command_count = 0;

        while (cmd != NULL)
        {
            cmd = trim_spaces(cmd);
            if (*cmd != '\0')
            {
                commands[command_count++] = strdup(cmd);
            }
            cmd = strtok(NULL, "&");
        }

        if (command_count > 0 && k == 0)
        {
            char *last_cmd = commands[command_count - 1];
            execute_commands(last_cmd, 0);
            free(last_cmd);
        }

        for (int i = 0; i < command_count - 1; i++)
        {
            execute_commands(commands[i], 1);
            free(commands[i]);
        }
        if (k == 1)
        {
            execute_commands(commands[command_count - 1], 1);
        }
        command = strtok_r(NULL, ";", &saveptr);
    }
}

void fab(const char *input)
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    display_input = strdup(input);

    input_copy = strdup(input);
    if (input_copy == NULL)
    {
        fprintf(stderr, RED_TEXT "Memory allocation error\n" RESET_COLOR);
        exit(EXIT_FAILURE);
    }
    if (strchr(input_copy, '&') != 0)
    {
        var = 1;
    }
    else
    {
        var = 0;
    }
    // printf("%s\n", input_copy);
    process_input(input_copy);
    free(input_copy);
}
