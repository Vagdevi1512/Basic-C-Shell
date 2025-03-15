#ifndef PROCESSES_H
#define PROCESSES_H


#include <sys/types.h> // For pid_t

// Structure for background processes
typedef struct bg_process
{
    pid_t pid;
    char command[256];
    struct bg_process *next;
} bg_process_t;

// Structure for foreground processes
typedef struct fg_process
{
    pid_t pid;
    char command[256];
    struct fg_process *next;
    int stopped;
} fg_process_t;

typedef struct stopped_process
{
    pid_t pid;
    char command[256];
    struct stopped_process *next;
} stopped_process_t;

typedef struct bg_process2
{
    pid_t pid;
    char command[256];
    struct bg_process2 *next;
} bg_process_t2;

// int stopped_process_count = 0;

// External declarations of global variables
extern bg_process_t *bg_list_head;
extern bg_process_t2 *bg_list_head2;
extern fg_process_t *fg_list_head;
extern stopped_process_t *stopped_process_head;

// Function prototypes
void sigchld_handler(int signum);

void add_fg_process(pid_t pid, const char *command);
void add_bg_process(pid_t pid, const char *command);
void add_bg_process2(pid_t pid, const char *command);
void add_stopped_process(pid_t pid, const char *command);

void remove_fg_process(pid_t pid);
void remove_bg_process(pid_t pid);
void remove_bg_process2(pid_t pid);
void stop_all_background_processes();
#endif