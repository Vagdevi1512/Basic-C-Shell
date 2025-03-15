#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#define MAX_PATH_LENGTH 4096
#define COLOR_RESET    "\033[0m"
#define COLOR_BLUE     "\033[34m"
#define COLOR_GREEN    "\033[32m"
#define COLOR_WHITE    "\033[37m"
#define COLOR_RED      "\033[31m"
#define MAX_ENTRIES    1024
#define BLOCK_SIZE     4096  // Assuming 4KB block size as fallback

typedef struct {
    char name[MAX_PATH_LENGTH];
    struct stat statbuf;
    int is_directory;
} DirectoryEntry;

static const char* format_time(time_t time);

// Function to expand '~' to the home directory
static void expand_tilde_path(const char *input_path, char *resolved_path) {
    if (input_path[0] == '~') {
        const char *home_dir = getenv("HOME"); // Get home directory from environment variable
        if (!home_dir) {
            struct passwd *pw = getpwuid(getuid()); // Fallback to getpwuid if HOME is not set
            home_dir = pw ? pw->pw_dir : NULL;
        }

        if (home_dir) {
            snprintf(resolved_path, MAX_PATH_LENGTH, "%s%s", home_dir, input_path + 1); // Concatenate home and the remaining path
        } else {
            fprintf(stderr, COLOR_RED "ERROR: HOME environment variable is not set\n" COLOR_RESET);
            strncpy(resolved_path, input_path, MAX_PATH_LENGTH - 1); // Fallback to the original path if HOME is not set
        }
    } else {
        strncpy(resolved_path, input_path, MAX_PATH_LENGTH - 1); // Copy the path as is if it doesn't start with '~'
    }
    resolved_path[MAX_PATH_LENGTH - 1] = '\0'; // Null-terminate the resolved path
}

// Comparator function for qsort
static int compare_entries(const void *a, const void *b) {
    const DirectoryEntry *entry_a = (const DirectoryEntry *)a;
    const DirectoryEntry *entry_b = (const DirectoryEntry *)b;
    return strcmp(entry_a->name, entry_b->name);
}

static void print_info(const DirectoryEntry *entry, int detailed_info) {
    const char *color = COLOR_WHITE;

    if (entry->is_directory) {
        color = COLOR_BLUE;
    } else if (entry->statbuf.st_mode & S_IXUSR) {
        color = COLOR_GREEN;
    }

    if (detailed_info) {
        char permissions[11];
        snprintf(permissions, sizeof(permissions), "%c%c%c%c%c%c%c%c%c%c",
                 (S_ISDIR(entry->statbuf.st_mode)) ? 'd' : '-',
                 (entry->statbuf.st_mode & S_IRUSR) ? 'r' : '-',
                 (entry->statbuf.st_mode & S_IWUSR) ? 'w' : '-',
                 (entry->statbuf.st_mode & S_IXUSR) ? 'x' : '-',
                 (entry->statbuf.st_mode & S_IRGRP) ? 'r' : '-',
                 (entry->statbuf.st_mode & S_IWGRP) ? 'w' : '-',
                 (entry->statbuf.st_mode & S_IXGRP) ? 'x' : '-',
                 (entry->statbuf.st_mode & S_IROTH) ? 'r' : '-',
                 (entry->statbuf.st_mode & S_IWOTH) ? 'w' : '-',
                 (entry->statbuf.st_mode & S_IXOTH) ? 'x' : '-');

        printf("%s%-10s %lu %s %s %5ld %s %s %s%s\n",
               color, permissions, entry->statbuf.st_nlink,
               getpwuid(entry->statbuf.st_uid)->pw_name,
               getgrgid(entry->statbuf.st_gid)->gr_name,
               entry->statbuf.st_size,
               format_time(entry->statbuf.st_mtime),
               color, entry->name, COLOR_RESET);
    } else {
        printf("%s%s%s\n", color, entry->name, COLOR_RESET);
    }
}

static const char* format_time(time_t time) {
    static char buffer[32];
    struct tm *tm_info = localtime(&time);
    strftime(buffer, sizeof(buffer), "%b %d %H:%M", tm_info);
    return buffer;
}

static void calculate_total_blocks(const char *path, int show_hidden, int *total_blocks) {
    struct dirent *entry;
    DIR *dp = opendir(path);
    struct stat statbuf;
    struct statvfs vfs;
    int block_size = BLOCK_SIZE; // Fallback to 4096 bytes

    if (statvfs(path, &vfs) == 0) {
        block_size = vfs.f_frsize; // Use the filesystem's fragment size
    }

    if (dp == NULL) {
        fprintf(stderr, COLOR_RED "ERROR: Unable to open directory: %s\n" COLOR_RESET, path);
        return;
    }

    *total_blocks = 0;

    while ((entry = readdir(dp))) {
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }

        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (lstat(full_path, &statbuf) == 0) {
            *total_blocks += (statbuf.st_blocks * block_size) / BLOCK_SIZE;  // Calculate based on system's block size
        } else {
            fprintf(stderr, COLOR_RED "ERROR: Getting stats for file %s failed: %s\n" COLOR_RESET, full_path, strerror(errno));
        }
    }

    closedir(dp);
}

static void print_file_info(const char *path) {
    struct stat statbuf;

    if (lstat(path, &statbuf) != 0) {
        fprintf(stderr, COLOR_RED "ERROR: lstat failed for %s: %s\n" COLOR_RESET, path, strerror(errno));
        return;
    }

    char permissions[11];
    snprintf(permissions, sizeof(permissions), "%c%c%c%c%c%c%c%c%c%c",
             (S_ISDIR(statbuf.st_mode)) ? 'd' : '-',
             (statbuf.st_mode & S_IRUSR) ? 'r' : '-',
             (statbuf.st_mode & S_IWUSR) ? 'w' : '-',
             (statbuf.st_mode & S_IXUSR) ? 'x' : '-',
             (statbuf.st_mode & S_IRGRP) ? 'r' : '-',
             (statbuf.st_mode & S_IWGRP) ? 'w' : '-',
             (statbuf.st_mode & S_IXGRP) ? 'x' : '-',
             (statbuf.st_mode & S_IROTH) ? 'r' : '-',
             (statbuf.st_mode & S_IWOTH) ? 'w' : '-',
             (statbuf.st_mode & S_IXOTH) ? 'x' : '-');

    printf("%s%-10s %lu %s %s %5ld %s %s%s\n",
           (S_ISDIR(statbuf.st_mode)) ? COLOR_BLUE : COLOR_WHITE,
           permissions, statbuf.st_nlink,
           getpwuid(statbuf.st_uid)->pw_name,
           getgrgid(statbuf.st_gid)->gr_name,
           statbuf.st_size,
           format_time(statbuf.st_mtime),
           path, COLOR_RESET);
}

static void list_entries(const char *path, int show_hidden, int detailed_info, int list_directories_only) {
    struct dirent *entry;
    DIR *dp = opendir(path);
    DirectoryEntry entries[MAX_ENTRIES];
    int count = 0;
    int total_blocks = 0;

    if (dp == NULL) {
        fprintf(stderr, COLOR_RED "ERROR: Unable to open directory: %s\n" COLOR_RESET, path);
        return;
    }

    // Calculate and print total blocks if detailed_info is enabled
    if (detailed_info) {
        calculate_total_blocks(path, show_hidden, &total_blocks);
        printf("total %d\n", total_blocks);
    }

    // Read all entries
    while ((entry = readdir(dp))) {
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }

        struct stat statbuf;
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (lstat(full_path, &statbuf) == 0) {
            if (list_directories_only && !S_ISDIR(statbuf.st_mode)) {
                continue;
            }

            strncpy(entries[count].name, entry->d_name, sizeof(entries[count].name) - 1);
            entries[count].name[sizeof(entries[count].name) - 1] = '\0';
            entries[count].statbuf = statbuf;
            entries[count].is_directory = S_ISDIR(statbuf.st_mode);
            count++;
        } else {
            fprintf(stderr, COLOR_RED "ERROR: Getting stats for file %s failed: %s\n" COLOR_RESET, full_path, strerror(errno));
        }
    }

    closedir(dp);

    // Sort using qsort
    qsort(entries, count, sizeof(DirectoryEntry), compare_entries);

    for (int i = 0; i < count; i++) {
        print_info(&entries[i], detailed_info);
    }
}

void list_files(const char *path, int show_hidden, int detailed_info) {
    struct stat statbuf;
    char resolved_path[MAX_PATH_LENGTH];

    expand_tilde_path(path, resolved_path);

    if (lstat(resolved_path, &statbuf) != 0) {
        fprintf(stderr, COLOR_RED "ERROR: lstat failed for %s: %s\n" COLOR_RESET, resolved_path, strerror(errno));
        return;
    }

    if (S_ISDIR(statbuf.st_mode)) {
        list_entries(resolved_path, show_hidden, detailed_info, 0);
    } else {
        print_file_info(resolved_path);
    }
}

void list_directories(const char *path, int show_hidden, int detailed_info) {
    char resolved_path[MAX_PATH_LENGTH];
    expand_tilde_path(path, resolved_path);
    list_entries(resolved_path, show_hidden, detailed_info, 1);
}
