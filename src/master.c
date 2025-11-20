#include "master.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static int enqueue_connection(shared_data_t *shared, int sockfd) {
    connection_queue_t *q = &shared->queue;
    if (q->count == MAX_QUEUE_SIZE) return -1;
    q->sockets[q->rear] = sockfd;
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    q->count++;
    return 0;
}

static void send_503(int sockfd) {
    const char* response = 
        "HTTP/1.1 503 Service Unavailable\r\n"
        "Content-Length: 19\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Service Unavailable";
    write(sockfd, response, strlen(response));
}

int start_master(int port, shared_data_t* shared, semaphores_t* sems) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 128) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    printf("Master listening on port %d...\n", port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }

        // Try enqueue without blocking (reject if full)
        if (sem_trywait(sems->empty_slots) == -1) {
            send_503(client_fd);
            close(client_fd);
            continue;
        }

        sem_wait(sems->queue_mutex);
        if (enqueue_connection(shared, client_fd) != 0) {
            send_503(client_fd);
            close(client_fd);
            sem_post(sems->empty_slots);
        }
        sem_post(sems->queue_mutex);
        sem_post(sems->filled_slots);
    }

    close(server_fd);
    return 0;
}
