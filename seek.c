#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ftw.h>
#include "hop.h"
#include "seek.h"

int seek_error = 0;

int is_file = 0, is_dir = 0, execute_flag = 0;
char *target_name = NULL;
char *target_directory = "."; // Default directory
int match_count = 0;
char *single_match_path = NULL;
int single_file_found = 0;
int single_dir_found = 0;

// Color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_ERROR    "\033[31m" // Red for errors
#define COLOR_SUCCESS  "\033[32m" // Green for success
#define COLOR_INFO     "\033[34m" // Blue for info

// Function to check if a filename matches the target prefix
int matches_target(const char *filename)
{
    if (target_name == NULL)
        return 0;
    return strncmp(filename, target_name, strlen(target_name)) == 0;
}

// Function to handle matches when only one is found
void handle_single_match()
{
    if (match_count == 1 && single_match_path)
    {
        struct stat sb;
        if (stat(single_match_path, &sb) == 0)
        {
            if (execute_flag && S_ISREG(sb.st_mode))
            {
                if (access(single_match_path, R_OK) == 0)
                {
                    if (sb.st_mode & S_IXUSR)
                    {
                        // Execute the file
                        execl(single_match_path, single_match_path, (char *)NULL);
                        perror(COLOR_ERROR "Execution failed" COLOR_RESET);
                    }
                    else
                    {
                        // Print the file content
                        FILE *file = fopen(single_match_path, "r");
                        if (file)
                        {
                            char ch;
                            while ((ch = fgetc(file)) != EOF)
                                putchar(ch);
                            fclose(file);
                        }
                        else
                        {
                            perror(COLOR_ERROR "Failed to open file" COLOR_RESET);
                        }
                    }
                }
                else
                {
                    printf(COLOR_ERROR "Missing permissions for task!" COLOR_RESET "\n");
                }
            }
            else if (execute_flag && S_ISDIR(sb.st_mode))
            {
                if (access(single_match_path, X_OK) == 0)
                {
                    if (chdir(single_match_path) == 0)
                    {
                        printf(COLOR_INFO "Opened directory: %s" COLOR_RESET "\n", single_match_path);
                        seek_error = 1;
                        hop(single_match_path);
                        // seek_error = 1;
                    }
                    else
                    {
                        perror(COLOR_ERROR "Failed to open directory" COLOR_RESET);
                    }
                    // To jump to the
                    // hop(single_match_path);
                }
                else
                {
                    printf(COLOR_ERROR "Missing permissions for task!" COLOR_RESET "\n");
                }
            }
        }
        else
        {
            perror(COLOR_ERROR "Stat failed" COLOR_RESET);
        }
    }
}

// Callback function for nftw
int process_entry(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    if (!fpath)
        return 0; // Validate fpath

    const char *name = strrchr(fpath, '/');
    name = name ? (name + 1) : fpath;

    if (matches_target(name))
    {
        if ((is_file && S_ISREG(sb->st_mode)) || (is_dir && S_ISDIR(sb->st_mode)) || (!is_file && !is_dir))
        {
            if (match_count == 0)
            {
                free(single_match_path); // Free previous allocation
                single_match_path = strdup(fpath);
                if (!single_match_path)
                {
                    perror(COLOR_ERROR "strdup failed" COLOR_RESET);
                    return -1; // Handle error
                }
            }
            match_count++;
            if (S_ISDIR(sb->st_mode))
            {
                single_dir_found = 1;
                printf(COLOR_INFO "%s/\033[0m\n", fpath); // Blue for directories
            }
            else if (S_ISREG(sb->st_mode))
            {
                single_file_found = 1;
                printf(COLOR_SUCCESS "%s" COLOR_RESET "\n", fpath); // Green for files
            }
        }
    }

    return 0;
}

int seek_files(const char *input)
{
    char input_copy[1024];
    strncpy(input_copy, input, sizeof(input_copy) - 1);
    input_copy[sizeof(input_copy) - 1] = '\0';

    // while (1) {
    // Reset state for each new input
    is_file = 0;
    is_dir = 0;
    execute_flag = 0;
    match_count = 0;
    single_file_found = 0;
    single_dir_found = 0;

    free(single_match_path); // Free previous allocation
    single_match_path = NULL;

    // printf("Enter search parameters (or 'exit' to quit): ");
    // if (fgets(input, sizeof(input), stdin) == NULL) {
    //     perror("Error reading input");
    //     return 1;
    // }

    // Remove newline character
    size_t len = strlen(input_copy);
    if (len > 0 && input_copy[len - 1] == '\n')
    {
        input_copy[len - 1] = '\0';
    }

    // Exit condition
    // if (strcmp(input, "exit") == 0) {
    //     break;
    // }

    // Parse input
    char *token = strtok(input_copy, " ");
    while (token != NULL)
    {
        if (token[0] == '-')
        {
            for (int j = 1; token[j] != '\0'; j++)
            {
                if (token[j] == 'f')
                    is_file = 1;
                else if (token[j] == 'd')
                    is_dir = 1;
                else if (token[j] == 'e')
                    execute_flag = 1;
                else
                {
                    printf(COLOR_ERROR "Invalid flags!" COLOR_RESET "\n");
                    return 1;
                }
            }
        }
        else
        {
            if (!target_name)
                target_name = token;
            else
                target_directory = token;
        }
        token = strtok(NULL, " ");
    }

    if (is_file && is_dir)
    {
        printf(COLOR_ERROR "Invalid flags!" COLOR_RESET "\n");
        return 1;
    }

    if (!target_name)
    {
        printf(COLOR_ERROR "Target name is required!" COLOR_RESET "\n");
        return 1;
    }

// Use FTW_PHYS if defined; otherwise, use FTW_MOUNT
#if defined(FTW_PHYS)
    if (nftw(target_directory, process_entry, 20, FTW_PHYS) == -1)
    {
#else
    if (nftw(target_directory, process_entry, 20, FTW_MOUNT) == -1)
    {
#endif
        perror(COLOR_ERROR "nftw" COLOR_RESET);
        return 1;
    }

    if (execute_flag && (single_file_found || single_dir_found))
    {
        handle_single_match();
    }

    // Free target name if it was dynamically allocated
    target_name = NULL;
    // }

    // free(single_match_path); // Final free before exiting
    return 0;
}
