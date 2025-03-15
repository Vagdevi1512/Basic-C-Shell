#include "reveal.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define MAX_PATH_LENGTH 1024
char oldpwd[MAX_PATH_LENGTH]; // Store the previous directory
int oldpwd_is_set;            // Flag to track if OLDPWD is valid
#define MAX_ENTRIES 1024      // Max number of directory entries we can handle

typedef struct {
    char name[MAX_PATH_LENGTH];
    struct stat statbuf;
    int is_directory;
} DirectoryEntry;

#define COLOR_RESET    "\033[0m"
#define COLOR_RED      "\033[31m"
#define COLOR_BLUE     "\033[1;34m"
#define COLOR_GREEN    "\033[1;32m"
#define COLOR_WHITE    "\033[1;37m"

// Function to remove leading dot for sorting
void shift_left_if_dot(char *name) {
    if (name[0] == '.') {
        memmove(name, name + 1, strlen(name)); // Shift the string one character to the left
    }
}

// Comparison function for qsort
int compare_entries(const void *a, const void *b) {
    DirectoryEntry *entryA = (DirectoryEntry *)a;
    DirectoryEntry *entryB = (DirectoryEntry *)b;
    return strcmp(entryA->name, entryB->name);
}

static void list_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    DirectoryEntry entries[MAX_ENTRIES];
    int entry_count = 0;

    if ((dir = opendir(path)) == NULL) {
        fprintf(stderr, COLOR_RED "Error: Unable to open directory %s.\n" COLOR_RESET, path);
        return;
    }

    // Read all entries into an array
    while ((entry = readdir(dir)) != NULL && entry_count < MAX_ENTRIES) {
        if (entry->d_name[0] == '.') {
            continue; // Skip hidden files
        }

        DirectoryEntry dir_entry;
        strncpy(dir_entry.name, entry->d_name, MAX_PATH_LENGTH - 1);
        dir_entry.name[MAX_PATH_LENGTH - 1] = '\0';

        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        // Use stat to get file information
        if (stat(full_path, &dir_entry.statbuf) == 0) {
            dir_entry.is_directory = S_ISDIR(dir_entry.statbuf.st_mode);
            entries[entry_count++] = dir_entry;
        } else {
            fprintf(stderr, COLOR_RED "Error: Unable to get information for %s.\n" COLOR_RESET, full_path);
        }
    }

    closedir(dir);

    // Sort the array of directory entries using qsort
    qsort(entries, entry_count, sizeof(DirectoryEntry), compare_entries);

    // Display the sorted entries
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].is_directory) {
            // Directory (blue)
            printf(COLOR_BLUE "%s/\033[0m\n", entries[i].name);
        } else if (entries[i].statbuf.st_mode & S_IXUSR) {
            // Executable (green)
            printf(COLOR_GREEN "%s*\033[0m\n", entries[i].name);
        } else {
            // Regular file (white)
            printf(COLOR_WHITE "%s\033[0m\n", entries[i].name);
        }
    }
}

void reveal(const char *path) {
    char resolved_path[MAX_PATH_LENGTH];
    char cwd[MAX_PATH_LENGTH];

    // Get current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, COLOR_RED "Error: Unable to get current working directory.\n" COLOR_RESET);
        return;
    }

    // Check which path to reveal
    if (strcmp(path, "h") == 0) {
        // Only update OLDPWD if not navigating to the previous directory
        strncpy(oldpwd, cwd, sizeof(oldpwd) - 1);
        oldpwd[sizeof(oldpwd) - 1] = '\0';
        oldpwd_is_set = 1;
        return;
    } else if (strcmp(path, "~") == 0) {
        const char *home = getenv("C_SHELL_HOME");
        if (home == NULL) {
            fprintf(stderr, COLOR_RED "Error: C_SHELL_HOME is not set.\n" COLOR_RESET);
            return;
        }
        strncpy(resolved_path, home, sizeof(resolved_path) - 1);
        resolved_path[sizeof(resolved_path) - 1] = '\0';
    } else if (strcmp(path, "..") == 0) {
        // Get parent directory without changing the current directory
        strncpy(resolved_path, cwd, sizeof(resolved_path) - 1);
        resolved_path[sizeof(resolved_path) - 1] = '\0';
        char *last_slash = strrchr(resolved_path, '/');
        if (last_slash != NULL) {
            *last_slash = '\0'; // Remove the last segment to get the parent directory
        } else {
            fprintf(stderr, COLOR_RED "Error: Could not determine parent directory.\n" COLOR_RESET);
            return;
        }
    } else if (strcmp(path, ".") == 0) {
        // Reveal current directory
        strncpy(resolved_path, cwd, sizeof(resolved_path) - 1);
        resolved_path[sizeof(resolved_path) - 1] = '\0';
    } else if (strcmp(path, "-") == 0) {
        // Reveal the previous directory (OLDPWD)
        if (!oldpwd_is_set) {
            fprintf(stderr, COLOR_RED "Error: No previous directory accessed.\n" COLOR_RESET);
            return;
        }
        strncpy(resolved_path, oldpwd, sizeof(resolved_path) - 1);
        resolved_path[sizeof(resolved_path) - 1] = '\0';
    } else {
        // Default to the specified path
        strncpy(resolved_path, path, sizeof(resolved_path) - 1);
        resolved_path[sizeof(resolved_path) - 1] = '\0';
    }

    // Update OLDPWD before listing the directory contents

    // List the directory contents in lexicographic order
    if (strcmp(path, "h") != 0)
        list_directory(resolved_path);
}
