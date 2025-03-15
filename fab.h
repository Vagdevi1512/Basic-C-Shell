#ifndef FAB_H
#define FAB_H

#include <time.h>

void fab(const char *input);
extern int background_job_count;
void execute_commands(char *command, int background);
void sigchld_handler(int signum);
extern time_t now;
extern time_t end;
extern double diff;
extern int time_diff;
extern char *display_input;

#endif // FAB_H