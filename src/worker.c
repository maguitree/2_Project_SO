#include "worker.h"
#include <stdio.h>
#include <unistd.h>

static int dequeue_connection(shared_data_t* shared, int* sockfd) {
    connection_queue_t* q = &shared->queue;
    if (q->count == 0) return -1;
    *sockfd = q->sockets[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->count--;
    return 0;
}

int start_worker(shared_data_t* shared, semaphores_t* sems) {
    while (1) {
        sem_wait(sems->filled_slots);
        sem_wait(sems->queue_mutex);

        int client_fd;
        if (dequeue_connection(shared, &client_fd) != 0) {
            sem_post(sems->queue_mutex);
            sem_post(sems->empty_slots);
            continue;
        }

        sem_post(sems->queue_mutex);
        sem_post(sems->empty_slots);

        // TODO: handle HTTP request here
        printf("Worker handling connection %d\n", client_fd);
        close(client_fd);
    }
    return 0;
}
