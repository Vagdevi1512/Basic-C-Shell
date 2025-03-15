#include "processes.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>


#define RED_TEXT "\033[31m"
#define RESET_COLOR "\033[0m"

// Find a stopped process by PID
stopped_process_t* find_stopped_process(pid_t pid) {
    stopped_process_t *current = stopped_process_head;

    while (current != NULL) {
        if (current->pid == pid) {
            return current; // Process with the given PID found
        }
        current = current->next; // Move to the next node in the list
    }

    return NULL; // Process with the given PID not found
}

// Remove a stopped process from the list
void remove_stopped_process(pid_t pid) {
    stopped_process_t *current = stopped_process_head;
    stopped_process_t *previous = NULL;

    // Traverse the list to find the process with the given PID
    while (current != NULL && current->pid != pid) {
        previous = current;
        current = current->next;
    }

    // If the process was not found
    if (current == NULL) {
        printf("Process with PID %d not found in stopped list\n", pid);
        return;
    }

    // Remove the process from the list
    if (previous == NULL) {
        // If the process is the head of the list
        stopped_process_head = current->next;
    } else {
        // Bypass the current node
        previous->next = current->next;
    }

    // Free the memory allocated for the removed process
    free(current);

    printf("Process with PID %d removed from stopped list\n", pid);
}

// Bring a process to the foreground
void fg(int pid) {
    stopped_process_t *proc = find_stopped_process(pid);
    if (proc) {
        tcsetpgrp(STDIN_FILENO, proc->pid);
        kill(proc->pid, SIGCONT); // Continue the process if it was stopped
        printf("Process %d brought to foreground\n", pid);
        
        // Remove the process from stopped processes and add it to foreground list
        remove_stopped_process(pid);
        add_fg_process(proc->pid, proc->command); // Add to foreground processes
    } else {
        printf("No such process found\n");
    }
}

// Resume a stopped process in the background
void bg(int pid) {
    stopped_process_t *proc = find_stopped_process(pid);
    if (proc->pid >= 0) {
        kill(proc->pid, SIGCONT); // Continue the process if it was stopped
        printf("Process %d resumed in the background\n", pid);
        
        // Remove the process from stopped processes and add it to background list
        remove_stopped_process(pid);
        add_bg_process(proc->pid, proc->command); // Add to background processes
    } else {
        // printf("No such process found\n");
         printf(RED_TEXT "Error sending signal: %s\n" RESET_COLOR, strerror(errno));
    }
}
