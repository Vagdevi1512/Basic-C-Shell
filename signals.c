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
#include "activities.h"
#include "iMan.h"
#include "processes.h"
#include "signals.h"

void sigint_handler(int signum)
{
    if (fg_list_head != NULL)
    {
        if (fg_list_head->command != NULL)
        {
            kill(fg_list_head->pid, SIGINT);
        }
    }
    printf("\n");
    return;
}

void sigterm_handler(int signum)
{
    stop_all_background_processes(); // Call function to handle background processes
    exit(0);                         // Exit the program
}

void sigtstp_handler(int signum)
{
    if (fg_list_head != NULL)
    {
        pid_t pid = fg_list_head->pid;
        if (kill(pid, SIGTSTP) == 0)
        {
            fflush(stdout);
            add_stopped_process(pid, fg_list_head->command);
            remove_fg_process(pid);
            if (kill(pid, SIGCONT) == 0)
            {
                fflush(stdout);
            }
        }
    }
    else
    {
        fflush(stdout);
    }
    printf("\n");
    return;
}
