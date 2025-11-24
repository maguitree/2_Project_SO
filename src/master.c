#include "master.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "ipc.h"

int start_master(int port, shared_data_t* shared, semaphores_t* sems, int channels[][2]) {
    (void)shared;
    (void)sems;
    
    int current_worker = 0;
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
        if (client_fd < 0) continue;

        
        if (send_fd(channels[current_worker][0], client_fd) == -1) {
            perror("send_fd");
        }
        
        close(client_fd);

        // 3. Round Robin: Select next worker
        current_worker = (current_worker + 1) % 4; 
    }

    close(server_fd);
    return 0;
}
