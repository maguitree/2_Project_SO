#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      // for fork, close, etc.
#include <sys/types.h>   // for pid_t
#include <signal.h>      // for kill, SIGTERM
#include <sys/wait.h>    
#include <errno.h>      
#include <sys/socket.h>

#include "ipc.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "master.h"
#include "worker.h"

#define NUM_WORKERS 4
#define SERVER_PORT 8080

static shared_data_t* shared = NULL;
static semaphores_t sems;
static pid_t workers[NUM_WORKERS];
int worker_channels[NUM_WORKERS][2];

// Unused parameter 'signo' is required by the signal handler function signature.
void cleanup(int signo) {
    (void)signo;
    printf("\nShutting down server...\n");

    // Kill worker processes
    for (int i = 0; i < NUM_WORKERS; i++) {
        // workers[i] > 0 check ensures we only try to kill successfully forked processes
        if (workers[i] > 0) {
            // Send the termination signal (SIGTERM)
            if (kill(workers[i], SIGTERM) == -1 && errno != ESRCH) {
                // ESRCH (No such process) is expected if the worker already exited.
                perror("kill");
            }
        }
    }

    // Wait for all workers to exit
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (workers[i] > 0) {
            waitpid(workers[i], NULL, 0);
        }
    }

    // Cleanup IPC resources
    destroy_semaphores(&sems);
    destroy_shared_memory(shared);

    printf("Server terminated gracefully.\n");
    exit(0);
}

int main() {
    // Handle Ctrl+C (SIGINT) gracefully
    signal(SIGINT, cleanup);

    // Create shared memory
    shared = create_shared_memory();
    if (!shared) {
        perror("create_shared_memory");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores
    if (init_semaphores(&sems, MAX_QUEUE_SIZE) < 0) {
        perror("init_semaphores");
        // Only destroy shared memory if semaphores failed, as cleanup will handle both on SIGINT
        destroy_shared_memory(shared);
        exit(EXIT_FAILURE);
    }

    // Fork worker processes
    for (int i = 0; i < NUM_WORKERS; i++) {
        // 1. Create the tunnel (socketpair) BEFORE forking
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, worker_channels[i]) < 0) {
            perror("socketpair");
            exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) { /* handle error */ }

        if (pid == 0) {
            // WORKER PROCESS
            close(worker_channels[i][0]); // Close Master's end
            
            // 2. Pass the tunnel FD (worker_channels[i][1]) to the worker
            return start_worker(shared, &sems, worker_channels[i][1]); 
        }
        
        // MASTER PROCESS
        workers[i] = pid;
        close(worker_channels[i][1]); // Close Worker's end
    }

    // 3. Pass the whole array to Master so it can pick who to talk to
    return start_master(SERVER_PORT, shared, &sems, worker_channels);
}