#ifndef PIPES_H
#define PIPES_H

int parse_command(char *command, char *argv[]);
void execute_pipeline(char *pipeline);
void pipes(const char *command) ;
void fg_command(int pid);
void bg_command(int pid);
extern int dummy;

#endif