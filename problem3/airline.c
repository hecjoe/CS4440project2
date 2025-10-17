// airline.c
// problem 3: airline passengers

// build: make airline
// run:   ./airline 100 3 5 2

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

typedef struct passenger
{
    int id;
    sem_t advance; // posted by a stage when this pax may proceed to the next stage
} passenger_t;

typedef struct
{
    passenger_t **q;
    int cap, head, tail, count;
    pthread_mutex_t lock;
    sem_t items; // counts queued passengers (workers wait on this)
} pax_queue_t;

typedef struct
{
    // three stage queues
    pax_queue_t q_bag, q_sec, q_board;
    int P, B, S, F;

    // arrival barrier: make all passengers "arrive at the same time"
    pthread_barrier_t arrive_barrier;

    // final count
    int seated_count;
    pthread_mutex_t seated_lock;
    int takeoff_announced;
} airport_t;

static void q_init(pax_queue_t *q, int cap)
{
    q->q = (passenger_t **)malloc(sizeof(passenger_t *) * cap);
    q->cap = cap;
    q->head = q->tail = q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    sem_init(&q->items, 0, 0);
}
static void q_push(pax_queue_t *q, passenger_t *p)
{
    pthread_mutex_lock(&q->lock);
    q->q[q->tail] = p;
    q->tail = (q->tail + 1) % q->cap;
    q->count++;
    pthread_mutex_unlock(&q->lock);
    sem_post(&q->items);
}
static passenger_t *q_pop(pax_queue_t *q)
{
    sem_wait(&q->items);
    pthread_mutex_lock(&q->lock);
    passenger_t *p = q->q[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count--;
    pthread_mutex_unlock(&q->lock);
    return p;
}

static void tiny_work() { usleep(100); }

// ---------------- worker roles ----------------
static void *baggage_worker(void *arg)
{
    airport_t *ap = (airport_t *)arg;
    for (;;)
    {
        passenger_t *p = q_pop(&ap->q_bag);
        printf("Passenger #%d is being processed by a baggage handler.\n", p->id);
        tiny_work();
        // allow passenger to proceed to security
        sem_post(&p->advance);
    }
    return NULL;
}
static void *security_worker(void *arg)
{
    airport_t *ap = (airport_t *)arg;
    for (;;)
    {
        passenger_t *p = q_pop(&ap->q_sec);
        printf("Passenger #%d is being screened by a security screener.\n", p->id);
        tiny_work();
        sem_post(&p->advance);
    }
    return NULL;
}
static void *boarding_worker(void *arg)
{
    airport_t *ap = (airport_t *)arg;
    for (;;)
    {
        passenger_t *p = q_pop(&ap->q_board);
        printf("Passenger #%d is being seated by a flight attendant.\n", p->id);
        tiny_work();

        pthread_mutex_lock(&ap->seated_lock);
        ap->seated_count++;
        int done = ap->seated_count;
        pthread_mutex_unlock(&ap->seated_lock);

        printf("Passenger #%d has been seated and relaxes.\n", p->id);

        if (done == ap->P)
        {
            // print takeoff once
            if (__sync_bool_compare_and_swap(&ap->takeoff_announced, 0, 1))
            {
                printf("*** All %d passengers are seated. The plane takes off! ***\n", ap->P);
            }
        }
        sem_post(&p->advance); // release passenger thread to exit
    }
    return NULL;
}

// ---------------- passenger threads ----------------
static void *passenger_thread(void *arg)
{
    airport_t *ap = (airport_t *)arg;
    passenger_t self;
    self.id = (int)(size_t)pthread_getspecific((pthread_key_t)0);
    self.id = (int)(size_t)arg;
    sem_init(&self.advance, 0, 0);

    printf("Passenger #%d arrived at the terminal.\n", self.id);
    // arrive together
    pthread_barrier_wait(&ap->arrive_barrier);

    // stage 1: baggage
    printf("Passenger #%d is waiting at baggage processing for a handler.\n", self.id);
    q_push(&ap->q_bag, &self);
    sem_wait(&self.advance);

    // stage 2: security
    printf("Passenger #%d is waiting to be screened by a screener.\n", self.id);
    q_push(&ap->q_sec, &self);
    sem_wait(&self.advance);

    // stage 3: boarding
    printf("Passenger #%d is waiting to board the plane by an attendant.\n", self.id);
    q_push(&ap->q_board, &self);
    sem_wait(&self.advance);

    sem_destroy(&self.advance);
    return NULL;
}

static void usage(const char *p)
{
    fprintf(stderr, "Usage: %s <P passengers> <B handlers> <S screeners> <F attendants>\n", p);
    fprintf(stderr, "Example: %s 100 3 5 2\n", p);
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        usage(argv[0]);
        return 2;
    }
    airport_t ap;
    memset(&ap, 0, sizeof(ap));

    ap.P = atoi(argv[1]);
    ap.B = atoi(argv[2]);
    ap.S = atoi(argv[3]);
    ap.F = atoi(argv[4]);

    if (ap.P <= 0 || ap.B <= 0 || ap.S <= 0 || ap.F <= 0)
    {
        usage(argv[0]);
        return 2;
    }

    q_init(&ap.q_bag, ap.P);
    q_init(&ap.q_sec, ap.P);
    q_init(&ap.q_board, ap.P);
    pthread_mutex_init(&ap.seated_lock, NULL);
    pthread_barrier_init(&ap.arrive_barrier, NULL, ap.P);
    ap.takeoff_announced = 0;

    // create workers first: B, S, F (in that order)
    pthread_t *bag = (pthread_t *)malloc(sizeof(pthread_t) * ap.B);
    pthread_t *sec = (pthread_t *)malloc(sizeof(pthread_t) * ap.S);
    pthread_t *brd = (pthread_t *)malloc(sizeof(pthread_t) * ap.F);

    for (int i = 0; i < ap.B; i++)
        pthread_create(&bag[i], NULL, baggage_worker, &ap);
    for (int i = 0; i < ap.S; i++)
        pthread_create(&sec[i], NULL, security_worker, &ap);
    for (int i = 0; i < ap.F; i++)
        pthread_create(&brd[i], NULL, boarding_worker, &ap);

    // then create P passenger threads
    pthread_t *pax = (pthread_t *)malloc(sizeof(pthread_t) * ap.P);
    for (int i = 0; i < ap.P; i++)
    {
        // pass ID via casted integer pointer-sized value
        pthread_create(&pax[i], NULL, passenger_thread, (void *)(size_t)(i + 1));
    }

    // wait for all passengers to exit (workers run forever; program ends when passengers done)
    for (int i = 0; i < ap.P; i++)
        pthread_join(pax[i], NULL);

    printf("All passenger threads completed. Exiting.\n");
    return 0;
}
