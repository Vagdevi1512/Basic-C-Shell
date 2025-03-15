#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include "display.h"
#include "hop.h"
#include "input.h"
#include "errno.h"
#include "log.h"
#include "proclore.h"
#include "fab.h"
#include "signals.h"
#include "activities.h"
#include "iMan.h"
#include "processes.h"

int waiting_for_input = 1;
#define INPUT_BUFFER_SIZE 1024
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
// stopped_process_t *stopped_process_head = NULL;
int k;

// void sigint_handler(int signum)
// {
//     if (fg_list_head != NULL)
//     {
//         if (fg_list_head->command != NULL)
//         {
//             kill(fg_list_head->pid, SIGINT);
//         }
//     }
//     printf("\n");
//     return;
// }

// void sigterm_handler(int signum)
// {
//     stop_all_background_processes(); // Call function to handle background processes
//     exit(0);                         // Exit the program
// }

// void sigtstp_handler(int signum)
// {
//     if (fg_list_head != NULL)
//     {
//         pid_t pid = fg_list_head->pid;
//         if (kill(pid, SIGTSTP) == 0)
//         {
//             fflush(stdout);
//             add_stopped_process(pid, fg_list_head->command);
//             remove_fg_process(pid);
//             if (kill(pid, SIGCONT) == 0)
//             {
//                 fflush(stdout);
//             }
//         }
//     }
//     else
//     {
//         fflush(stdout);
//     }
//     printf("\n");
//     return;
// }

char shell_directory[1024] = {0};

int main()
{
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
    {
        fprintf(stderr, COLOR_RED "ERROR: Unable to determine the directory of the shell executable.\n" COLOR_RESET);
        exit(EXIT_FAILURE);
    }
    exe_path[len] = '\0';
    strcpy(shell_directory, dirname(exe_path)); // Get the directory of the executable

    initialize_log_file(shell_directory);

    set_home_directory();
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        fprintf(stderr, COLOR_RED "ERROR: Unable to get the current working directory.\n" COLOR_RESET);
        exit(EXIT_FAILURE);
    }
    setenv("C_SHELL_HOME", cwd, 1); // Set the environment variable for C shell code location

    struct sigaction sa;

    // Handle SIGINT (Ctrl-C)
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // Handle SIGTSTP (Ctrl-Z)
    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTSTP, &sa, NULL);

    // Handle SIGCHLD
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    // signal(SIGINT,sigint_handler);
    // signal(SIGTSTP,sigtstp_handler);
    // signal(SIGTERM,sigterm_handler);

    while (1)
    {
        print_prompt();
        char input[INPUT_BUFFER_SIZE];
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            if (feof(stdin))
            { // Handle Ctrl-D (EOF)
                printf("\nEOF received. Stopping all background processes...\n");
                stop_all_background_processes();
                break; // Exit the loop and terminate the shell
            }
            // perror("Error reading input");
            continue; // Continue reading input
        }

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n')
        {
            input[len - 1] = '\0'; // Remove newline character
        }

        if (strcmp(input, "exit") == 0)
        {
            log_new_command(input);
            printf("Exiting shell...\n");
            stop_all_background_processes(); // Clean up background processes
            break;                           // Exit the loop and terminate the shell
        }
        else if (strcmp(input, "activities") == 0)
        {
            log_new_command(input);
            display_all_processes(); // Print all processes
        }
        else if (strncmp(input, "iMan", 4) == 0)
        {
            log_new_command(input);
            iMan(input); // Process the iMan command
        }
        else
        {
            int k=strlen(input);
            handle_input(input, 1,k); // Process other user input
        }
    }

    // Free dynamically allocated memory for stopped processes
    stopped_process_t *temp;
    while (stopped_process_head != NULL)
    {
        temp = stopped_process_head;
        stopped_process_head = stopped_process_head->next;
        free(temp);
    }

    // Free dynamically allocated memory for foreground processes
    fg_process_t *fg_temp;
    while (fg_list_head != NULL)
    {
        fg_temp = fg_list_head;
        fg_list_head = fg_list_head->next;
        free(fg_temp);
    }

    // Free dynamically allocated memory for background processes
    bg_process_t *bg_temp;
    while (bg_list_head != NULL)
    {
        bg_temp = bg_list_head;
        bg_list_head = bg_list_head->next;
        free(bg_temp);
    }

    return 0;
}
