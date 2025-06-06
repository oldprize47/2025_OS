#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_LEN 20
#define THREADS 100
typedef struct _rwlock_t {
    sem_t lock;
    sem_t writelock;
    sem_t timelock;
    sem_t wait_writer;
    int readers;
} rwlock_t;

typedef struct {
    int id;
    int duration;
} thread_arg_t;

rwlock_t rw;
struct timeval start, end;

double get_time() {
    gettimeofday(&end, NULL);
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}
void rwlock_init(rwlock_t *rw) {
    rw->readers = 0;
    sem_init(&rw->lock, 0, 1);
    sem_init(&rw->writelock, 0, 1);
    sem_init(&rw->timelock, 0, 1);
    sem_init(&rw->wait_writer, 0, 1);
};

void rwlock_acquire_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->readers++;
    if (rw->readers == 1)
        sem_wait(&rw->writelock);
    sem_post(&rw->lock);
}
void rwlock_release_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0)
        sem_post(&rw->writelock);
    sem_post(&rw->lock);
}
void rwlock_acquire_writelock(rwlock_t *rw) {
    sem_wait(&rw->writelock);
}
void rwlock_release_writelock(rwlock_t *rw) {
    sem_post(&rw->writelock);
}

void rwlock_acquire_waitwriter(rwlock_t *rw) {
    sem_wait(&rw->wait_writer);
}

void rwlock_release_waitwriter(rwlock_t *rw) {
    sem_post(&rw->wait_writer);
}

void rwlock_acquire_timelock(rwlock_t *rw) {
    sem_wait(&rw->timelock);
}

void rwlock_release_timelock(rwlock_t *rw) {
    sem_post(&rw->timelock);
}

// reader의 의사코드
void *reader(void *arg) {
    // Get a thread argument that contains reader’s ID and processing time
    thread_arg_t *targ = (thread_arg_t *)arg;
    int id = targ->id;
    int duration = targ->duration;

    rwlock_acquire_timelock(&rw);
    printf("[%.4f] Reader#%d: Created!\n", get_time(), id);  // Print the thread creation status
    rwlock_release_timelock(&rw);

    rwlock_acquire_waitwriter(&rw);
    rwlock_release_waitwriter(&rw);
    rwlock_acquire_readlock(&rw);  // rw is a structure for reader-writer Lock
    rwlock_acquire_timelock(&rw);
    printf("[%.4f] Reader#%d: Read started! (reading %d ms)\n", get_time(), id, duration);  // Print status indicating the start of reading
    rwlock_release_timelock(&rw);
    usleep(duration * 1000);  // Sleep for the specified processing time

    rwlock_acquire_timelock(&rw);
    printf("[%.4f] Reader#%d: Terminated!\n", get_time(), id);  // Print the thread termination status
    rwlock_release_timelock(&rw);
    rwlock_release_readlock(&rw);
    free(arg);
}

// writer의 의사코드
void *writer(void *arg) {
    // Get a thread argument that contains writer’s ID and processing time
    thread_arg_t *targ = (thread_arg_t *)arg;
    int id = targ->id;
    int duration = targ->duration;
    rwlock_acquire_timelock(&rw);
    printf("[%.4f] Writer#%d: Created!\n", get_time(), id);  // Print the thread creation status
    rwlock_release_timelock(&rw);

    rwlock_acquire_waitwriter(&rw);  // rw is a structure for reader-writer Lock
    rwlock_acquire_writelock(&rw);
    rwlock_acquire_timelock(&rw);
    printf("[%.4f] Writer#%d: Write started! (writing %d ms)\n", get_time(), id, duration);  // Print status indicating the start of writing
    rwlock_release_timelock(&rw);
    usleep(duration * 1000);  // Sleep for the specified processing time
    rwlock_acquire_timelock(&rw);
    printf("[%.4f] Writer#%d: Terminated!\n", get_time(), id);  // Print the thread termination status
    rwlock_release_timelock(&rw);
    rwlock_release_writelock(&rw);
    rwlock_release_waitwriter(&rw);
    free(arg);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <sequence file>\n", argv[0]);
        exit(1);
    }

    char str[MAX_LEN];
    pthread_t thread[THREADS];
    int ret;
    int num = 0, r = 1, w = 1;
    char type;
    int duration = 0;
    FILE *fs = fopen(argv[1], "r");  // Open the sequence file
    if (!fs) {
        perror("fopen");
        exit(1);
    }
    rwlock_init(&rw);  // Initialize RW lock
    gettimeofday(&start, NULL);
    while (fscanf(fs, " %c %d", &type, &duration) == 2) {  // while (read a line from sequence file) & Extract the thread type and processing time from the line
        thread_arg_t *arg = malloc(sizeof(thread_arg_t));  // Allocate memory for the thread argument
        if (type == 'R') {                                 // if (type is ‘R’)
            arg->id = r;
            r++;
            arg->duration = duration;
            ret = pthread_create(&thread[num], NULL, &reader, arg);
            if (ret) {
                printf("error pthread_create\n");
                exit(1);
            }
        } else if (type == 'W') {  // else if (type is ‘W’)
            arg->id = w;
            w++;
            arg->duration = duration;
            ret = pthread_create(&thread[num], NULL, &writer, arg);
            if (ret) {
                printf("error pthread_create\n");
                exit(1);
            }
        } else {
            printf("error file format\n");
            exit(1);
        }
        num++;
        usleep(100000);  // sleep 100ms
    }
    for (int i = 0; i < num; i++) {
        ret = pthread_join(thread[i], NULL);
        if (ret) {
            printf("error pthread_join\n");
            exit(1);
        }
    }

    return 0;
}