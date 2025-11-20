#ifndef MASTER_H
#define MASTER_H

#include "shared_mem.h"
#include "semaphores.h"

int start_master(int port, shared_data_t* shared, semaphores_t* sems);

#endif
