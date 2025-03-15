#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "fab.h"
#include "input.h"
#include "ping.h"
#include "processes.h"

#define RED_TEXT "\033[31m"
#define RESET_COLOR "\033[0m"

void ping_command(int pid, int signal_number)
{
    int signal = signal_number % 32; // Ensure valid signal number in range 1-31
    int process_found = 0;
    char temp_command[256]; // Temporary variable to store the command

    bg_process_t *bg_curr = bg_list_head;
    fg_process_t *fg_curr = fg_list_head;
    stopped_process_t *stopped_curr = stopped_process_head;

    // Check in background processes list
    while (bg_curr != NULL)
    {
        if (bg_curr->pid == pid)
        {
            process_found = 1;
            strncpy(temp_command, bg_curr->command, sizeof(temp_command) - 1);
            temp_command[sizeof(temp_command) - 1] = '\0'; // Ensure null-termination
            break;
        }
        bg_curr = bg_curr->next;
    }

    // Check in foreground processes list if not found in background
    if (!process_found)
    {
        while (fg_curr != NULL)
        {
            if (fg_curr->pid == pid)
            {
                process_found = 1;
                strncpy(temp_command, fg_curr->command, sizeof(temp_command) - 1);
                temp_command[sizeof(temp_command) - 1] = '\0'; // Ensure null-termination
                break;
            }
            fg_curr = fg_curr->next;
        }
    }

    // Check in stopped processes list if not found in background or foreground
    if (!process_found)
    {
        while (stopped_curr != NULL)
        {
            if (stopped_curr->pid == pid)
            {
                process_found = 1;
                break;
            }
            stopped_curr = stopped_curr->next;
        }
    }

    // If process not found
    if (!process_found)
    {
        // printf("No such process found\n");
        printf(RED_TEXT "No such process found\n" RESET_COLOR);
        return;
    }

    if (signal == SIGSTOP)
    {
        printf("Stopping process with pid %d (SIGSTOP)\n", pid);
        remove_bg_process(pid);
        remove_bg_process2(pid);
        remove_fg_process(pid);
        add_stopped_process(pid, temp_command);
        // Send the signal and stop the process
        if (kill(pid, SIGSTOP) == 0)
        {
            printf("Process with pid %d stopped.\n", pid);
        }
        else
        {
            // printf("Error sending signal: %s\n", strerror(errno));
            printf(RED_TEXT "Error sending signal: %s\n" RESET_COLOR, strerror(errno));
        }
        return;
    }

    // Send other signals to the process
    if (kill(pid, signal) == 0)
    {
        printf("Sent signal %d to process with pid %d\n", signal, pid);
    }
    else
    {
        // printf("Error sending signal: %s\n", strerror(errno));
        printf(RED_TEXT "Error sending signal: %s\n" RESET_COLOR, strerror(errno));
    }
}
