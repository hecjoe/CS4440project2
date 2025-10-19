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
  
- Problem 2: Task order is strictly adhered to by loop structures and semapahores, which ensures children aren't given tasks out of sequence.
By utilizing a queue, the Father thread processes children in the exact order they were bathed by the Mother thread. 
The day_start_sem semaphore creates a handoff between days so the mother blocks until the father signals completion of the last day.
There was also observable concurrency when the Mother continued bathing the children while the Father simultaneously read to and tucked the now bathed children.
  
- Problem 3: The worker pools and blocking queues avoid spin; arrivals are gated by the barrier; the plane takes off when seated_count is equal to P.
