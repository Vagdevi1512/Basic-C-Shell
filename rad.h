#ifndef RAD_H
#define RAD_H

// rad.h
// int handle_redirection(char *cmd, int *input_fd, int *output_fd, int *prev_fd, int is_last_command);
int parse(char *command, char **argv);
void rad(const char *cmd);

#endif