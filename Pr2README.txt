# CS4440project2
Project 2 - Exercises in Concurrency and Synchronization

Files:
- producer_consumer.c  : Problem 1 – bounded buffer with pthreads + semaphores
- mh.c                 : Problem 2 – Mother/Father handoff with day cycles
- airline.c            : Problem 3 – multi-stage passenger processing with worker pools
- Makefile             : builds all targets
- typescript.txt       : record of program test runs

-------------------------------------------------------------------------------------

Build:
  gcc -Wall -Wextra -O2 -pthread -o producer_consumer producer_consumer.c
  gcc -Wall -Wextra -O2 -pthread -o mh mh.c
  gcc -Wall -Wextra -O2 -pthread -o airline airline.c
  
-------------------------------------------------------------------------------------

Run:
producer_consumer.c
  # Baseline (shows “working” state; defaults: BUF=5, ITEMS=20, 200ms/200ms)
  ./producer_consumer

mh.c
  ./mh <number of days>

airline.c
  /.airline <passengers> <baggage handlers> <security screeners> <flight attendants>
  
-------------------------------------------------------------------------------------

Observations:
- Problem 1: The control program initializes buffer and synchronization variables, creates and terminates producer and consumer threads after processing all items. 
The producer generates characters into the buffer, and the consumer retrieves and prints them. Semaphores manage buffer states and synchronization, ensuring balanced 
production and consumption.
  
- Problem 2: Task order is strictly adhered to by loop structures and semapahores, which ensures children aren't given tasks out of sequence.
By utilizing a queue, the Father thread processes children in the exact order they were bathed by the Mother thread. 
The day_start_sem semaphore creates a handoff between days so the mother blocks until the father signals completion of the last day.
There was also observable concurrency when the Mother continued bathing the children while the Father simultaneously read to and tucked the now bathed children.
  
- Problem 3: Output shows passengers announcing arrival in a non-deterministic order, then immediately queuing at baggage, confirming simultaneous “arrival” followed 
by first-stage waiting. Work then pipelines correctly: while early passengers move from baggage → screening → boarding, later passengers keep joining the earlier queues, 
and each passenger prints “has been seated and relaxes” before the final takeoff line. The single takeoff message appears only after the last seating message, 
demonstrating proper synchronization and no premature departure.
