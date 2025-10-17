# Project 2 Makefile

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -pthread

all: producer_consumer mh airline

producer_consumer: producer_consumer.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

mh: mh.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

airline: airline.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

clean:
	rm -f producer_consumer mh airline

.PHONY: all clean
