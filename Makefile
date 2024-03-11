# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Werror -std=c99

smalloc.o: smalloc.c
	$(CC) $(CFLAGS) -O0 -c smalloc.c

# Clean up intermediate files and executable
clean:
	rm -rf *.o