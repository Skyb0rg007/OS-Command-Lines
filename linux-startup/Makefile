
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -g -fno-builtin
LDFLAGS = -nostartfiles -g -nostdlib -nolibc -nodefaultlibs
LDLIBS =

parrot: parrot.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

parrot.o: parrot.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) parrot.o parrot
