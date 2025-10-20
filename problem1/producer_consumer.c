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
char get_char(void)
{
    static char c = 'A';
    if (c > 'Z')
        c = 'A';
    return c++;
}

// Consumer function that simulates using a character
// This function prints the consumed character to the console
void use_char(char c)
{
    printf("Consumed: %c\n", c);
    fflush(stdout);
}

// Function to print current buffer contents
// '_' represents an empty slot; letters represent filled slots
// This helps visualize buffer states for full, empty, and partially filled
void print_buffer()
{
    printf("Buffer: [");
    for (int i = 0; i < MAX_SIZE; i++)
    {
        int index = i;
        int count_in_buffer = produced_count - consumed_count;
        if ((index >= out && index < out + count_in_buffer) ||
            (out + count_in_buffer > MAX_SIZE && index < (out + count_in_buffer) % MAX_SIZE))
            printf("%c", buffer[i]);
        else
            printf("_");
        if (i < MAX_SIZE - 1)
            printf(" ");
    }
    printf("]\n");
    fflush(stdout);
}

// Producer thread
// Produces characters and adds them to the buffer if space is available
// Stops after producing TOTAL_ITEMS characters
void *producer(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        if (produced_count >= TOTAL_ITEMS)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);

        char item = get_char();

        // Shows when producer is waiting because the buffer is full
        int sem_val;
        sem_getvalue(&empty, &sem_val);
        if (sem_val == 0)
        {
            printf("Producer waiting: buffer full\n");
            fflush(stdout);
        }

        // Waits for an empty slot
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);

        // Adds item to buffer
        // Places the item in the buffer at the 'in' index
        buffer[in] = item;
        in = (in + 1) % MAX_SIZE;
        produced_count++;

        printf("Produced: %c (Total produced: %d)\n", item, produced_count);
        print_buffer(); // Show buffer state after production

        pthread_mutex_unlock(&mutex);
        sem_post(&full);

        // Slows production to better visualize buffer alternation
        usleep(200000);
    }
    return NULL;
}

// Consumer thread
// Consumes characters from the buffer if available
// Stops after consuming TOTAL_ITEMS characters
void *consumer(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        if (consumed_count >= TOTAL_ITEMS)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);

        // Shows when consumer is waiting because the buffer is empty
        int sem_val;
        sem_getvalue(&full, &sem_val);
        if (sem_val == 0)
        {
            printf("Consumer waiting: buffer empty\n");
            fflush(stdout);
        }

        // Waits for a filled slot
        sem_wait(&full);
        pthread_mutex_lock(&mutex);

        // Removes item from buffer
        // Takes the item from the buffer at the 'out' index and updates the index
        char item = buffer[out];
        out = (out + 1) % MAX_SIZE;
        consumed_count++;

        use_char(item);
        print_buffer(); // Show buffer state after consumption

        pthread_mutex_unlock(&mutex);
        sem_post(&empty);

        // Slows consumption to better visualize buffer alternation
        usleep(200000);
    }
    return NULL;
}

int main(void)
{
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

    // Cleanup: destroys mutex and semaphores
    pthread_mutex_destroy(&mutex);
    sem_destroy(&full);
    sem_destroy(&empty);

    printf("%d items produced and consumed. Exiting program.\n", TOTAL_ITEMS);

    return 0;
}
