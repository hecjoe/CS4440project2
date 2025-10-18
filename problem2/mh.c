// PROJECT 2
// mh.c
// Mother and Father taking care of 12 children over multiple days.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_CHILDREN 12

static int ready_queue[NUM_CHILDREN];
static int q_head = 0, q_tail = 0;

static pthread_mutex_t q_mutex = PTHREAD_MUTEX_INITIALIZER;

// counts how many children are ready (bathed) for Father to process.
static sem_t ready_sem;

// day coordination
static sem_t day_start_sem;  // Mother waits here at the start of each day

static int num_cycles = 0;

// push child id (1-12) into the ready queue
static void queue_push(int child_id) {
    pthread_mutex_lock(&q_mutex);
    ready_queue[q_tail] = child_id;
    q_tail = (q_tail + 1) % NUM_CHILDREN;
    pthread_mutex_unlock(&q_mutex);
    sem_post(&ready_sem); // signal Father that one child is ready
}

// pop next ready child id
static int queue_pop(void) {
    sem_wait(&ready_sem); // blocks Father until at least one bathed child exists
    pthread_mutex_lock(&q_mutex);
    int child_id = ready_queue[q_head];
    q_head = (q_head + 1) % NUM_CHILDREN;
    pthread_mutex_unlock(&q_mutex);
    return child_id;
}

static void* mother_thread(void* arg) {
    for (int day = 1; day <= num_cycles; day++) {
        // Mother starts the day only when allowed.
        sem_wait(&day_start_sem);

        // Day print statement
        printf("This is day #%d of a day in the life of Mother Hubbard.\n", day);

        // Mother wakes up
		printf("Mother is waking up to take care of the children.\n");

        // Task 1: Wake up children 1-12
        for (int c = 1; c <= NUM_CHILDREN; c++) {
			printf("Child #%d is being woken up.\n", c);
            usleep(100);
        }

        // Task 2: Breakfast for all children
        for (int c = 1; c <= NUM_CHILDREN; c++) {
            printf("Child #%d is being fed breakfast.\n", c);
            usleep(100);
        }

        // Task 3: Send to school for all children 
        for (int c = 1; c <= NUM_CHILDREN; c++) {
            printf("Child #%d is being sent to school.\n", c);
            usleep(100);
        }

        // Task 4: Give dinner to all children
        for (int c = 1; c <= NUM_CHILDREN; c++) {
            printf("Child #%d is being given dinner.\n", c);
            usleep(100);
        }

        // Task 5: Give bath to all children
        for (int c = 1; c <= NUM_CHILDREN; c++) {
            printf("Child #%d is being given a bath.\n", c);
            usleep(100);
            queue_push(c); // signal Father with this child's id
        }

        // Mother naps after bathing all children and waits for Father to finish the day.
        printf("Mother is taking a nap break.\n");

        // Loop ends here
		// Mother will be woken by Father for the next day
    }
    return NULL;
}

static void* father_thread(void* arg) {
    for (int day = 1; day <= num_cycles; day++) {
        // Father starts the day asleep
        int processed = 0;
		int first_child = 1; // Flag to print wakeup message only once

        while (processed < NUM_CHILDREN) {
            int child_id = queue_pop();  // wakes Father when first child is bathed
            // Father processes only that specific child

            if (first_child) {
                printf("Father is waking up to help with the children.\n");
				first_child = 0;
            }

			// Task 6: Read a book and tuck in bed for the child
            printf("Child #%d is being read a book.\n", child_id);
            usleep(100);
            printf("Child #%d is being tucked in bed.\n", child_id);
            usleep(100);
            processed++;
        }

		// Day end print statement
        printf("This is the end of day #%d of a day in the life of Mother Hubbard.\n", day);
        
        // All children are in bed. Father goes to sleep and wakes Mother for next day.
        printf("Father is going to sleep and waking up Mother to take care of the children.\n");
        sem_post(&day_start_sem); // Wake Mother to start the next day
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_cycles>\n", argv[0]);
        return 1;
    }
    num_cycles = atoi(argv[1]);
    if (num_cycles <= 0) {
		fprintf(stderr, "Error: Number of cycles must be positive.\n");
		return 1;
    }

    // Init semaphores
    sem_init(&ready_sem, 0, 0);       // No children ready at start
    sem_init(&day_start_sem, 0, 1);   // Mother starts awake for day 1

    pthread_t mom, dad;
    pthread_create(&mom, NULL, mother_thread, NULL);
    pthread_create(&dad, NULL, father_thread, NULL);

    pthread_join(mom, NULL);
    pthread_join(dad, NULL);

    sem_destroy(&ready_sem);
    sem_destroy(&day_start_sem);

    printf("Finished after %d day(s).\n", num_cycles);
    return 0;
}
