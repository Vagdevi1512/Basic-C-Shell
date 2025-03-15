# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Source files
SRCS = display.c fab.c input.c list.c log.c hop.c proclore.c reveal.c seek.c main.c redirection.c pipes.c rad.c processes.c signals.c ping.c activities.c fgbg.c neonate.c iMan.c 

# Object files
OBJS = $(SRCS:.c=.o)

# Header files (dependencies)
HEADERS = display.h fab.h input.h list.h log.h hop.h proclore.h reveal.h seek.h redirection.h pipes.h rad.h processes.h  signals.h ping.h activities.h fgbg.h neonate.h iMan.h 

# Default target to build the executable
all: a.out

# Rule to build the executable from object files
a.out: $(OBJS)
	$(CC) $(CFLAGS) -o a.out $(OBJS)

# Rule to compile each source file into an object file
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f a.out $(OBJS)

# Run the program
run: a.out
	./a.out

.PHONY: all clean run
