#include "display.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "fab.h"
// #include "fab.h"
int time_diff;
double diff;

#define COLOR_RESET "\033[0m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"

#define MAX_PATH_LENGTH 1024
static char home_directory[MAX_PATH_LENGTH];

// Retrieves the username and system name.
static void get_user_and_system(char *username, char *systemname, size_t size)
{
    struct utsname uts;
    uname(&uts); // Get system information

    // Get username
    char *login_name = getlogin();
    if (login_name != NULL)
    {
        snprintf(username, size, "%s", login_name);
    }
    else
    {
        snprintf(username, size, "unknown");
    }

    // Get system name
    snprintf(systemname, size, "%s", uts.nodename);
}

// Sets the home directory.
void set_home_directory()
{
    if (getcwd(home_directory, sizeof(home_directory)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
}

// Formats the current directory path relative to the home directory.
char *format_path(const char *current_directory)
{
    static char path[MAX_PATH_LENGTH];
    if (strncmp(current_directory, home_directory, strlen(home_directory)) == 0)
    {
        snprintf(path, sizeof(path), "~%s", current_directory + strlen(home_directory));
    }
    else
    {
        snprintf(path, sizeof(path), "%s", current_directory);
    }
    return path;
}
// Prints the shell prompt with color.
void print_prompt(int last_cmd_time)
{
    char username[256];
    char systemname[256];
    char cwd[MAX_PATH_LENGTH];

    get_user_and_system(username, systemname, sizeof(username));
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    char *formatted_path = format_path(cwd);
    
    diff = difftime(end, now);
    time_diff = (int)diff;

    if (time_diff <= 2)
    {
        printf("%s<%s%s%s@%s%s:%s%s%s>%s ",
               COLOR_CYAN,
               COLOR_YELLOW, username, COLOR_RESET,
               COLOR_CYAN, systemname, COLOR_RESET,
               COLOR_GREEN, formatted_path, COLOR_RESET);
    }
    else
    {
        printf("%s<%s%s%s@%s%s:%s%s%s%s %s : %ds>%s ",
               COLOR_CYAN,
               COLOR_YELLOW, username, COLOR_RESET,
               COLOR_CYAN, systemname, COLOR_RESET,
               COLOR_GREEN, formatted_path, display_input, COLOR_RESET,
               time_diff, COLOR_RESET);
               time_diff=0;
    }
}

void display_output(const char *output)
{
    printf("%s\n", output);
}