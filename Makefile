# Project 2 Makefile

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -pthread

# build everything
all: producer_consumer mh airline

# problem 1 - bounded-buffer producer/consumer problem
producer_consumer: producer_consumer.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

# problem 2 - mother hubbard
mh: mh.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

# problem 3 - airline passengers
airline: airline.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

# tidy
clean:
	rm -f producer_consumer mh airline

.PHONY: all clean
