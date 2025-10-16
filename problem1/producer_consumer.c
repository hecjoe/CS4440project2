/* This program demonstrates the producer-consumer problem using pthreads and semaphores in C.
The producer generates characters (A-Z) and places them in a shared buffer,
while the consumer takes characters from the buffer and uses them by printing to the console.
The buffer has a maximum size of 5, and the program ensures proper synchronization
between producer and consumer threads using mutexes and semaphores.
The program stops after producing and consuming a total of 20 items. */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define MAX_SIZE 5
#define TOTAL_ITEMS 20 

// Shared buffer and indices
// in is the next position the producer will write to
// out is the next position the consumer will read from
// Max size limits how many items can be in the buffer at once, which is 5
char buffer[MAX_SIZE];
int in = 0, out = 0;
int produced_count = 0;
int consumed_count = 0;

// Synchronization primitives
// mutex makes sure only one thread can access the buffer at a time
// full counts the filled slots that are ready for consumption
// empty counts the empty slots that are ready for production
pthread_mutex_t mutex;
sem_t full, empty;

// Generates next printable character (A–Z looping)
// If the character exceeds 'Z', it wraps around to 'A'
char get_char(void) {
    static char c = 'A';
    if (c > 'Z') c = 'A';
    return c++;
}

// Consumer function that simulates using a character
// This just function prints the consumed character to the console
void use_char(char c) {
    printf("Consumed: %c\n", c);
    fflush(stdout);
}

// Producer thread
// This function runs in a loop, producing characters and adding them to the buffer when there is space and stopping after producing a total of 20 items.
void *producer(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        if (produced_count >= TOTAL_ITEMS) {
            pthread_mutex_unlock(&mutex);
            break; 
        }
        pthread_mutex_unlock(&mutex);
        char item = get_char();


        // Waits for an empty slot
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);

        // Adds item to buffer
        // Places the item in the buffer at the 'in' index
        buffer[in] = item;
        in = (in + 1) % MAX_SIZE;
        produced_count++;

        printf("Produced: %c (Total produced: %d)\n", item, produced_count);

        // Signals that a new item is available
        pthread_mutex_unlock(&mutex);
        sem_post(&full);

        // Slows the production down so you can see the alternation
        usleep(200000);
    }
    return NULL;
}

// Consumer thread
// This function runs in a loop, consuming characters from the buffer when available and stopping after consuming a total of 20 items.
void *consumer(void *arg) {
        while (1) {
        pthread_mutex_lock(&mutex);
        if (consumed_count >= TOTAL_ITEMS) {
            pthread_mutex_unlock(&mutex);
            break; 
        }
        pthread_mutex_unlock(&mutex);


        // Waits for a filled slot
        // Locks the mutex to ensure exclusive access to the buffer
        sem_wait(&full);
        pthread_mutex_lock(&mutex);

        // Removes item from buffer
        // Takes the item from the buffer at the 'out' index and updates the index
        char item = buffer[out];
        out = (out + 1) % MAX_SIZE;
        consumed_count++;

        use_char(item);

        // Unlocks the mutex and signals that a slot is now empty
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);

        // Slows the consumption down so you can see the alternation
        usleep(200000); 
    }
    return NULL;
}

int main(void) {
    pthread_t prod, cons;

    // Initializes mutex and semaphores
    pthread_mutex_init(&mutex, NULL);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, MAX_SIZE);

    // Creates threads for producer and consumer
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    // Waits for threads to finish
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    // Cleanup which destroys mutex and semaphores
    pthread_mutex_destroy(&mutex);
    sem_destroy(&full);
    sem_destroy(&empty);

    printf("%d items produced and consumed. Exiting program.\n", TOTAL_ITEMS);

    return 0;
}
