#include "processes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

bg_process_t *bg_list_head = NULL;
bg_process_t2 *bg_list_head2 = NULL;
fg_process_t *fg_list_head = NULL;
stopped_process_t *stopped_process_head = NULL;
// int background_job_count = 0;

void add_bg_process(pid_t pid, const char *command)
{
    bg_process_t *new_process = malloc(sizeof(bg_process_t));
    if (new_process == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    new_process->pid = pid;
    strncpy(new_process->command, command, sizeof(new_process->command) - 1);
    new_process->command[sizeof(new_process->command) - 1] = '\0';
    new_process->next = bg_list_head;
    bg_list_head = new_process;
}

void add_fg_process(pid_t pid, const char *command)
{
    fg_process_t *new_process = malloc(sizeof(fg_process_t));
    if (new_process == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    new_process->pid = pid;
    strncpy(new_process->command, command, sizeof(new_process->command) - 1);
    new_process->command[sizeof(new_process->command) - 1] = '\0';
    new_process->next = fg_list_head;
    fg_list_head = new_process;
}


void add_bg_process2(pid_t pid, const char *command)
{
    bg_process_t2 *new_process = malloc(sizeof(bg_process_t2));
    if (new_process == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    new_process->pid = pid;
    strncpy(new_process->command, command, sizeof(new_process->command) - 1);
    new_process->command[sizeof(new_process->command) - 1] = '\0';
    new_process->next = bg_list_head2;
    bg_list_head2 = new_process;
}

void add_stopped_process(pid_t pid, const char *command)
{
    stopped_process_t *new_process = malloc(sizeof(stopped_process_t));
    if (new_process == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    new_process->pid = pid;
    strncpy(new_process->command, command, sizeof(new_process->command) - 1);
    new_process->command[sizeof(new_process->command) - 1] = '\0';
    new_process->next = stopped_process_head;
    stopped_process_head = new_process;
}

void remove_fg_process(pid_t pid)
{
    fg_process_t *prev = NULL, *curr = fg_list_head;
    while (curr != NULL)
    {
        if (curr->pid == pid)
        {
            if (prev == NULL)
            {
                fg_list_head = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void remove_bg_process(pid_t pid)
{
    bg_process_t *prev = NULL, *curr = bg_list_head;
    while (curr != NULL)
    {
        if (curr->pid == pid)
        {
            if (prev == NULL)
            {
                bg_list_head = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void stop_all_background_processes()
{
    bg_process_t *current = bg_list_head;
    while (current != NULL)
    {
        if (kill(current->pid, SIGSTOP) == -1)
        {
            perror("Failed to stop background process");
        }
        current = current->next;
    }
}

void remove_bg_process2(pid_t pid)
{
    bg_process_t2 *prev = NULL, *curr = bg_list_head2;
    while (curr != NULL)
    {
        if (curr->pid == pid)
        {
            if (prev == NULL)
            {
                bg_list_head2 = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}
