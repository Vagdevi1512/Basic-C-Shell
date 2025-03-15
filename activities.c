#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processes.h"

typedef struct {
    int pid;
    char command[256];
    char state[16]; // "Running" or "Stopped"
} process_info_t;

// Comparison function for qsort (numerical order by PID)
int compare_processes_by_pid(const void *a, const void *b)
{
    process_info_t *processA = (process_info_t *)a;
    process_info_t *processB = (process_info_t *)b;
    return (processA->pid - processB->pid);
}

void display_all_processes()
{
    // Array to hold all processes
    process_info_t processes[1024];
    int count = 0;

    // Collect background processes (from both lists)
    bg_process_t *bg_current = bg_list_head;
    while (bg_current != NULL)
    {
        processes[count].pid = bg_current->pid;
        strcpy(processes[count].command, bg_current->command);
        strcpy(processes[count].state, "Running");
        count++;
        bg_current = bg_current->next;
    }

    bg_process_t2 *bg_current2 = bg_list_head2;
    while (bg_current2 != NULL)
    {
        processes[count].pid = bg_current2->pid;
        strcpy(processes[count].command, bg_current2->command);
        strcpy(processes[count].state, "Running");
        count++;
        bg_current2 = bg_current2->next;
    }

    // Collect stopped foreground processes
    stopped_process_t *stopped_current = stopped_process_head;
    while (stopped_current != NULL)
    {
        processes[count].pid = stopped_current->pid;
        strcpy(processes[count].command, stopped_current->command);
        strcpy(processes[count].state, "Stopped");
        count++;
        stopped_current = stopped_current->next;
    }

    // Sort the processes array by PID
    qsort(processes, count, sizeof(process_info_t), compare_processes_by_pid);

    // Print the sorted processes
    if (count == 0)
    {
        return;
    }

    for (int i = 0; i < count; i++)
    {
        printf("[%d] : %s - %s\n", processes[i].pid, processes[i].command, processes[i].state);
    }
}
