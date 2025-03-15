#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

void sigterm_handler(int signum);
void sigint_handler(int signum);
void sigtstp_handler(int signum);

#endif