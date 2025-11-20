#ifndef WORKER_H
#define WORKER_H

#include "shared_mem.h"
#include "semaphores.h"

int start_worker(shared_data_t* shared, semaphores_t* sems);

#endif
