#ifndef REDIRECTION_H
#define REDIRECTION_H

void apply_redirections(int *input_fd, int *output_fd);
void handle_redirections(char **args);
void redirection(const char *input);

#endif