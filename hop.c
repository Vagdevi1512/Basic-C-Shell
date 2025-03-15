#include "hop.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "reveal.h"
#include "seek.h"

#define MAX_PATH_LENGTH 1024

// Color codes
#define RED_TEXT "\033[31m"
#define RESET_COLOR "\033[0m"

int is_first_hop = 1;

static int is_directory_exists(const char *path)
{
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

void hop(const char *path)
{
    char resolved_path[MAX_PATH_LENGTH];
    char temp_path[MAX_PATH_LENGTH];
    char initial_directory[MAX_PATH_LENGTH];

    // Save the current working directory
    if (getcwd(initial_directory, sizeof(initial_directory)) == NULL)
    {
        fprintf(stderr, RED_TEXT "ERROR: getcwd failed\n" RESET_COLOR);
        return;
    }

    // Tokenize the input path on spaces to handle multiple hops
    strncpy(temp_path, path, sizeof(temp_path) - 1);
    temp_path[sizeof(temp_path) - 1] = '\0';

    char *token = strtok(temp_path, " ");
    while (token != NULL)
    {
        if (strcmp(token, "~") == 0)
        {
            const char *c_shell_home = getenv("C_SHELL_HOME");
            if (c_shell_home)
            {
                strncpy(resolved_path, c_shell_home, sizeof(resolved_path) - 1);
                resolved_path[sizeof(resolved_path) - 1] = '\0';
            }
            else
            {
                fprintf(stderr, RED_TEXT "ERROR: C_SHELL_HOME environment variable not set\n" RESET_COLOR);
                return;
            }
        }
        else if (strcmp(token, ".") == 0)
        {
            if (getcwd(resolved_path, sizeof(resolved_path)) == NULL)
            {
                fprintf(stderr, RED_TEXT "ERROR: getcwd failed\n" RESET_COLOR);
                return;
            }
        }
        else if (strcmp(token, "..") == 0)
        {
            if (getcwd(resolved_path, sizeof(resolved_path)) == NULL)
            {
                fprintf(stderr, RED_TEXT "ERROR: getcwd failed\n" RESET_COLOR);
                return;
            }
            if (chdir("..") != 0)
            {
                fprintf(stderr, RED_TEXT "ERROR: chdir failed\n" RESET_COLOR);
                return;
            }
            if (getcwd(resolved_path, sizeof(resolved_path)) == NULL)
            {
                fprintf(stderr, RED_TEXT "ERROR: getcwd failed\n" RESET_COLOR);
                return;
            }
        }
        else if (strcmp(token, "-") == 0)
        {
            if (is_first_hop)
            {
                // Do nothing if `-` is the first token
                is_first_hop = 0;
                // Reset the current directory to initial_directory
                snprintf(resolved_path, sizeof(resolved_path), "%s", initial_directory);
            }
            else
            {
                const char *path = "h";
                reveal(path);
                const char *prev_directory = getenv("OLDPWD");
                if (prev_directory)
                {
                    strncpy(resolved_path, prev_directory, sizeof(resolved_path) - 1);
                    resolved_path[sizeof(resolved_path) - 1] = '\0';
                }
                else
                {
                    fprintf(stderr, RED_TEXT "ERROR: OLDPWD not set\n" RESET_COLOR);
                    return;
                }
            }
        }
        else if (token[0] == '/')
        {
            strncpy(resolved_path, token, sizeof(resolved_path) - 1);
            resolved_path[sizeof(resolved_path) - 1] = '\0';
        }
        else if (strncmp(token, "~/", 2) == 0)
        {
            const char *c_shell_home = getenv("C_SHELL_HOME");
            if (c_shell_home)
            {
                snprintf(resolved_path, sizeof(resolved_path), "%s/%s", c_shell_home, token + 2);
            }
            else
            {
                fprintf(stderr, RED_TEXT "ERROR: C_SHELL_HOME environment variable not set\n" RESET_COLOR);
                return;
            }
        }
        else
        {
            if (getcwd(resolved_path, sizeof(resolved_path)) == NULL)
            {
                fprintf(stderr, RED_TEXT "ERROR: getcwd failed\n" RESET_COLOR);
                return;
            }
            strncat(resolved_path, "/", sizeof(resolved_path) - strlen(resolved_path) - 1);
            strncat(resolved_path, token, sizeof(resolved_path) - strlen(resolved_path) - 1);
        }

        if (!is_directory_exists(resolved_path) && seek_error == 0)
        {
            fprintf(stderr, RED_TEXT "ERROR: Directory '%s' does not exist\n" RESET_COLOR, resolved_path);
            return;
        }
        if (chdir(resolved_path) != 0 && seek_error == 0)
        {
            fprintf(stderr, RED_TEXT "ERROR: chdir failed\n" RESET_COLOR);
            return;
        }
        else if (seek_error == 1)
        {
            seek_error = 0;
        }

        token = strtok(NULL, " ");
    }

    // Print the final working directory
    if (getcwd(resolved_path, sizeof(resolved_path)) == NULL)
    {
        fprintf(stderr, RED_TEXT "ERROR: getcwd failed\n" RESET_COLOR);
        return;
    }
    printf("%s\n", resolved_path);

    // Update environment variables
    setenv("OLDPWD", initial_directory, 1);
    setenv("PWD", resolved_path, 1);
}
