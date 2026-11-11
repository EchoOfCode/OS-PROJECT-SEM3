/*
 * worker.h - Worker process management for CampusCompute
 */
#ifndef WORKER_H
#define WORKER_H

#include "common.h"
#include <sys/types.h>

int worker_run_normal(void);
int worker_run_fail(void);

#endif /* WORKER_H */
