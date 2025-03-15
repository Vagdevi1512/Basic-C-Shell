#ifndef DISPLAY_H
#define DISPLAY_H

#include <limits.h>
#include <stddef.h>

// Initializes the home directory.
void set_home_directory();
// static void get_user_and_system(char *username, char *systemname, size_t size);
// Formats the current directory path relative to the home directory.
char* format_path(const char *current_directory);
// Prints the shell prompt.
void print_prompt();
void display_output(const char *output);

#endif // DISPLAY_H