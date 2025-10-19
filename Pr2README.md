# CS4440project2
Project 2 - Exercises in Concurrency and Synchronization

Files:
- producer_consumer.c  : Problem 1 – bounded buffer with pthreads + semaphores
- mh.c                 : Problem 2 – Mother/Father handoff with day cycles
- airline.c            : Problem 3 – multi-stage passenger processing with worker pools
- Makefile             : builds all targets
- typescript.txt       : record of program test runs

Build:
  make

Run:
ADD RUN COMMANDS AND PARAMETERS

Observations:
- Problem 1:
  
- Problem 2:
  
- Problem 3: Worker pools + blocking queues avoid spin; arrivals gated by barrier; plane takes off when seated_count == P.
