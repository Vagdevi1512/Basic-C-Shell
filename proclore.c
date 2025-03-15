#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h> // For tcgetpgrp()

#define COLOR_RESET    "\033[0m"
#define COLOR_RED      "\033[31m"

int process_exists(pid_t pid) {
    char path[256];
    sprintf(path, "/proc/%d", pid);
    struct stat sts;
    if (stat(path, &sts) == -1 && errno == ENOENT) {
        return 0; // Process does not exist
    }
    return 1; // Process exists
}

void proclore(pid_t pid) {
    char path[256];
    char status = '?';
    char exec_path[256];
    long virtual_mem = 0;
    pid_t process_group = 0;
    char status_flag[3] = {0}; // Buffer for status flag

    // Check if the process exists
    if (!process_exists(pid)) {
        fprintf(stderr, COLOR_RED "Error: Process with PID %d does not exist.\n" COLOR_RESET, pid);
        return;
    }

    // Open and read /proc/[pid]/stat to get process status
    sprintf(path, "/proc/%d/stat", pid);
    FILE *stat_file = fopen(path, "r");
    if (stat_file) {
        fscanf(stat_file, "%*d %*s %c", &status);
        fclose(stat_file);
    } else {
        fprintf(stderr, COLOR_RED "Error: Unable to open /proc/%d/stat.\n" COLOR_RESET, pid);
        return;
    }

    // Determine process group
    process_group = getpgid(pid);

    // Open and read /proc/[pid]/statm to get virtual memory size
    sprintf(path, "/proc/%d/statm", pid);
    FILE *statm_file = fopen(path, "r");
    if (statm_file) {
        fscanf(statm_file, "%ld", &virtual_mem);
        fclose(statm_file);
    } else {
        fprintf(stderr, COLOR_RED "Error: Unable to open /proc/%d/statm.\n" COLOR_RESET, pid);
        virtual_mem = 0; // Default to 0 if we can't read the file
    }

    // Get executable path
    sprintf(path, "/proc/%d/exe", pid);
    ssize_t len = readlink(path, exec_path, sizeof(exec_path) - 1);
    if (len != -1) {
        exec_path[len] = '\0';
    } else {
        // Path is unknown if readlink fails
        snprintf(exec_path, sizeof(exec_path), "Path unknown");
    }

    // Determine the status flag
    if (status == 'R') {
        // Check if the process is in the foreground or background
        pid_t foreground_pgid = tcgetpgrp(STDIN_FILENO);
        if (foreground_pgid == process_group) {
            snprintf(status_flag, sizeof(status_flag), "R+");
        } else {
            snprintf(status_flag, sizeof(status_flag), "R");
        }
    } else if (status == 'S') {
        // Check if the process is in the foreground or background
        pid_t foreground_pgid = tcgetpgrp(STDIN_FILENO);
        if (foreground_pgid == process_group) {
            snprintf(status_flag, sizeof(status_flag), "S+");
        } else {
            snprintf(status_flag, sizeof(status_flag), "S");
        }
    } else if (status == 'Z') {
        // If status is 'Z', the process is a zombie
        snprintf(status_flag, sizeof(status_flag), "Z");
    } else {
        // Default to '?'
        snprintf(status_flag, sizeof(status_flag), "%c", status);
    }

    // Print the process information
    printf("pid : %d\n", pid);
    printf("process Status : %s\n", status_flag);
    printf("Process Group : %d\n", process_group);
    printf("Virtual memory : %ld\n", virtual_mem);
    printf("executable Path : %s\n", exec_path);
}
