#include "worker.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h> // Add this
#include <string.h> // Add this
#include "ipc.h"


int start_worker(shared_data_t* shared, semaphores_t* sems, int channel_fd) {
    (void)shared;
    (void)sems;

    while (1) {
        // 1. Wait for connection from Master (Blocks here until Master sends something)
        int client_fd = recv_fd(channel_fd);
        if (client_fd < 0) continue;

        printf("Worker handling connection %d\n", client_fd);

        // 2. Handle Request (teste para ver se funcionava, pode se tirar)
        char response[] = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 19\r\n"
            "\r\n"
            "<h1>It Works!</h1>\n";

        write(client_fd, response, sizeof(response) - 1);
        close(client_fd);
    }
    return 0;
}
