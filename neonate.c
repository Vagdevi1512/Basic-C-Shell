#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/wait.h>
#include <time.h>

// Global variables
volatile sig_atomic_t keep_running = 1;
pid_t most_recent_pid = -1;
time_t most_recent_time = 0;

// Function to update the PID of the most recently created process
void update_most_recent_pid()
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/proc");
    if (!dir)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            char *endptr;
            pid_t pid = strtol(entry->d_name, &endptr, 10);
            if (*endptr == '\0') // Valid PID
            {
                struct stat statbuf;
                char path[256];
                snprintf(path, sizeof(path), "/proc/%d", pid);

                if (stat(path, &statbuf) == 0)
                {
                    if (statbuf.st_ctime > most_recent_time)
                    {
                        most_recent_time = statbuf.st_ctime;
                        most_recent_pid = pid;
                    }
                }
            }
        }
    }

    closedir(dir);
}

// Function to print the PID every [time_arg] seconds
void print_pid_periodically(int time_arg)
{
    while (keep_running)
    {
        update_most_recent_pid();
        if (most_recent_pid != -1)
        {
            printf("Most recent PID: %d\n", most_recent_pid);
        }
        else
        {
            printf("Error retrieving PID.\n");
        }

        sleep(time_arg);
    }
}

// Function to handle non-blocking key press (for 'x' key)
void setup_nonblocking_input()
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void restore_input_mode()
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void neonate(int time_arg)
{
    // Set up non-blocking input
    setup_nonblocking_input();

    // Start the printing of PID in a separate process
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process to handle PID printing
        print_pid_periodically(time_arg);
        exit(0);
    }
    else if (pid > 0)
    {
        // Parent process to handle key press
        char c;
        while (keep_running)
        {
            if (read(STDIN_FILENO, &c, 1) == 1)
            {
                if (c == 'x')
                {
                    keep_running = 0;
                    kill(pid, SIGTERM); // Terminate the child process
                }
            }
        }

        // Wait for the child process to exit
        wait(NULL);
    }
    else
    {
        perror("fork");
        restore_input_mode();
        exit(EXIT_FAILURE);
    }

    // Restore input mode
    restore_input_mode();
}
