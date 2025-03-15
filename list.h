#ifndef LIST_H
#define LIST_H

#include <limits.h>
#include <stddef.h>

// Function prototypes
void list_files(const char *path, int show_hidden, int detailed_info);
void list_directories(const char *path, int show_hidden, int detailed_info);
void list_all_entries(const char *path, int show_hidden, int detailed_info);

#endif // LIST_H
