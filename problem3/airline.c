/*
 * Problem 3 -Airline Passengers
 *
 * Simulates an airport pipeline with threads and synchronization:
 *   P passenger threads flow through three staged worker pools:
 *   B baggage handlers, S security screeners, and F flight attendants.
 * Each stage uses a mutex-protected FIFO queue; workers block on semaphores
 * (no busy-waiting). A pthread barrier makes all passengers "arrive together."
 * The plane takes off only after all P passengers are seated, printed once.
 *
 * Build: gcc -Wall -Wextra -O2 -pthread -o airline airline.c
 * Usage: ./airline <P passengers> <B handlers> <S screeners> <F attendants>
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

/* ---------------- Portable counting semaphore ---------------- */
typedef struct {
    pthread_mutex_t m;
    pthread_cond_t  cv;
    int count;
} csem_t;

static void csem_init(csem_t *s, int initial) {
    pthread_mutex_init(&s->m, NULL);
    pthread_cond_init(&s->cv, NULL);
    s->count = initial;
}
static void csem_destroy(csem_t *s) {
    pthread_mutex_destroy(&s->m);
    pthread_cond_destroy(&s->cv);
}
static void csem_post(csem_t *s) {
    pthread_mutex_lock(&s->m);
    s->count++;
    pthread_cond_signal(&s->cv);
    pthread_mutex_unlock(&s->m);
}
static void csem_wait(csem_t *s) {
    pthread_mutex_lock(&s->m);
    while (s->count == 0)
        pthread_cond_wait(&s->cv, &s->m);
    s->count--;
    pthread_mutex_unlock(&s->m);
}

/* ---------------- Portable barrier (mutex + cond) ---------------- */
typedef struct {
    pthread_mutex_t m;
    pthread_cond_t  cv;
    int needed, arrived, phase;
} my_barrier_t;

static void my_barrier_init(my_barrier_t *b, int count) {
    pthread_mutex_init(&b->m, NULL);
    pthread_cond_init(&b->cv, NULL);
    b->needed = count; b->arrived = 0; b->phase = 0;
}
static void my_barrier_destroy(my_barrier_t *b) {
    pthread_mutex_destroy(&b->m);
    pthread_cond_destroy(&b->cv);
}
static void my_barrier_wait(my_barrier_t *b) {
    pthread_mutex_lock(&b->m);
    int ph = b->phase;
    if (++b->arrived == b->needed) {
        b->arrived = 0;
        b->phase++;
        pthread_cond_broadcast(&b->cv);
    } else {
        while (ph == b->phase) pthread_cond_wait(&b->cv, &b->m);
    }
    pthread_mutex_unlock(&b->m);
}

/* ---------------- Types ---------------- */
typedef struct passenger {
    int id;
    csem_t advance;   // posted by a stage when this pax may proceed
} passenger_t;

typedef struct {
    passenger_t **q;
    int cap, head, tail, count;
    pthread_mutex_t lock;
    csem_t items;     // counting semaphore for #items in queue
} pax_queue_t;

typedef struct {
    pax_queue_t q_bag, q_sec, q_board;
    int P, B, S, F;

    my_barrier_t arrive_barrier;

    int seated_count;
    pthread_mutex_t seated_lock;
    int takeoff_announced;
} airport_t;

/* ---------------- Queue helpers ---------------- */
static void q_init(pax_queue_t *q, int cap) {
    q->q = (passenger_t **)malloc(sizeof(passenger_t *) * (size_t)cap);
    q->cap = cap; q->head = q->tail = q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    csem_init(&q->items, 0);
}
static void q_push(pax_queue_t *q, passenger_t *p) {
    pthread_mutex_lock(&q->lock);
    q->q[q->tail] = p;
    q->tail = (q->tail + 1) % q->cap;
    q->count++;
    pthread_mutex_unlock(&q->lock);
    csem_post(&q->items);
}
static passenger_t* q_pop(pax_queue_t *q) {
    csem_wait(&q->items);
    pthread_mutex_lock(&q->lock);
    passenger_t *p = q->q[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count--;
    pthread_mutex_unlock(&q->lock);
    return p;
}

static void tiny_work(void) { usleep(100); }

/* ---------------- Worker roles ---------------- */
static void *baggage_worker(void *arg) {
    airport_t *ap = (airport_t *)arg;
    for (;;) {
        passenger_t *p = q_pop(&ap->q_bag);
        printf("Passenger #%d is being processed by a baggage handler.\n", p->id);
        tiny_work();
        csem_post(&p->advance);  // allow passenger to go to security
    }
    return NULL;
}
static void *security_worker(void *arg) {
    airport_t *ap = (airport_t *)arg;
    for (;;) {
        passenger_t *p = q_pop(&ap->q_sec);
        printf("Passenger #%d is being screened by a security screener.\n", p->id);
        tiny_work();
        csem_post(&p->advance);  // allow passenger to go to boarding
    }
    return NULL;
}
static void *boarding_worker(void *arg) {
    airport_t *ap = (airport_t *)arg;
    for (;;) {
        passenger_t *p = q_pop(&ap->q_board);
        printf("Passenger #%d is being seated by a flight attendant.\n", p->id);
        tiny_work();

        pthread_mutex_lock(&ap->seated_lock);
        ap->seated_count++;
        int done = ap->seated_count;
        pthread_mutex_unlock(&ap->seated_lock);

        printf("Passenger #%d has been seated and relaxes.\n", p->id);

        if (done == ap->P) {
            if (__sync_bool_compare_and_swap(&ap->takeoff_announced, 0, 1)) {
                printf("*** All %d passengers are seated. The plane takes off! ***\n", ap->P);
            }
        }
        csem_post(&p->advance); // release passenger thread to exit
    }
    return NULL;
}

/* ---------------- Passenger thread ---------------- */
typedef struct { airport_t *ap; int id; } pax_arg_t;

static void *passenger_thread(void *varg) {
    pax_arg_t *pa = (pax_arg_t *)varg;
    airport_t *ap = pa->ap;
    int id = pa->id;
    free(pa);

    passenger_t self;
    self.id = id;
    csem_init(&self.advance, 0);

    printf("Passenger #%d arrived at the terminal.\n", self.id);
    my_barrier_wait(&ap->arrive_barrier);

    printf("Passenger #%d is waiting at baggage processing for a handler.\n", self.id);
    q_push(&ap->q_bag, &self);
    csem_wait(&self.advance);

    printf("Passenger #%d is waiting to be screened by a screener.\n", self.id);
    q_push(&ap->q_sec, &self);
    csem_wait(&self.advance);

    printf("Passenger #%d is waiting to board the plane by an attendant.\n", self.id);
    q_push(&ap->q_board, &self);
    csem_wait(&self.advance);

    csem_destroy(&self.advance);
    return NULL;
}

/* ---------------- Main ---------------- */
static void usage(const char *p) {
    fprintf(stderr, "Usage: %s <P passengers> <B handlers> <S screeners> <F attendants>\n", p);
    fprintf(stderr, "Example: %s 100 3 5 2\n", p);
}

int main(int argc, char **argv) {
    if (argc != 5) { usage(argv[0]); return 2; }

    airport_t ap;
    memset(&ap, 0, sizeof(ap));

    ap.P = atoi(argv[1]);
    ap.B = atoi(argv[2]);
    ap.S = atoi(argv[3]);
    ap.F = atoi(argv[4]);
    if (ap.P <= 0 || ap.B <= 0 || ap.S <= 0 || ap.F <= 0) { usage(argv[0]); return 2; }

    q_init(&ap.q_bag, ap.P);
    q_init(&ap.q_sec, ap.P);
    q_init(&ap.q_board, ap.P);
    pthread_mutex_init(&ap.seated_lock, NULL);
    my_barrier_init(&ap.arrive_barrier, ap.P);
    ap.takeoff_announced = 0;

    // Create worker pools FIRST: B -> S -> F
    pthread_t *bag = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)ap.B);
    pthread_t *sec = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)ap.S);
    pthread_t *brd = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)ap.F);
    for (int i = 0; i < ap.B; i++) pthread_create(&bag[i], NULL, baggage_worker, &ap);
    for (int i = 0; i < ap.S; i++) pthread_create(&sec[i], NULL, security_worker, &ap);
    for (int i = 0; i < ap.F; i++) pthread_create(&brd[i], NULL, boarding_worker, &ap);

    // THEN create P passenger threads
    pthread_t *pax = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)ap.P);
    for (int i = 0; i < ap.P; i++) {
        pax_arg_t *pa = (pax_arg_t *)malloc(sizeof(pax_arg_t));
        pa->ap = &ap; pa->id = i + 1;
        pthread_create(&pax[i], NULL, passenger_thread, pa);
    }

    for (int i = 0; i < ap.P; i++) pthread_join(pax[i], NULL);

    printf("All passenger threads completed. Exiting.\n");

    my_barrier_destroy(&ap.arrive_barrier);
    return 0;
}

