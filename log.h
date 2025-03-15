#ifndef LOG_H
#define LOG_H

void initialize_log_file(const char *directory);
void log_commands();
void log_purge();
void log_execute(int index);
void log_new_command(const char *new_command);

#endif // LOG_H
