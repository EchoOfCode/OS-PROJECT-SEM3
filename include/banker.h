/*
 * banker.h - Banker's Algorithm for CampusCompute
 */
#ifndef BANKER_H
#define BANKER_H

#include "common.h"

#define BANKER_MAX_PROCESSES 64
#define BANKER_MAX_RESOURCES 64

typedef struct {
    int num_processes;
    int num_resources;
    int available[BANKER_MAX_RESOURCES];
    int max[BANKER_MAX_PROCESSES][BANKER_MAX_RESOURCES];
    int allocation[BANKER_MAX_PROCESSES][BANKER_MAX_RESOURCES];
    int need[BANKER_MAX_PROCESSES][BANKER_MAX_RESOURCES];
} BankerState;

/* Safety check */
bool banker_check_safety(int *available, int **allocation, int **maximum,
                         int n_proc, int n_res, int *safe_sequence);

/* Demos */
int banker_run_safe(void);
int banker_run_unsafe(void);

/* File loader */
bool banker_load_file(const char *path, int **available, int ***allocation,
                      int ***maximum, int *n_proc, int *n_res);

/* Print helper */
void banker_print_state(const BankerState *state);

#endif /* BANKER_H */
