#ifndef HOP_H
#define HOP_H

#define MAX_PATH_LENGTH 1024

extern char home_directory[MAX_PATH_LENGTH];

void hop(const char *path);
// static int is_directory_exists(const char *path);

#endif // HOP_H