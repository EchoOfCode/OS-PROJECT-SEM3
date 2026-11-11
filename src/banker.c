/*
 * banker.c - Banker's Safety Algorithm for CampusCompute
 *
 * Implements deadlock avoidance using Dijkstra's Banker's algorithm.
 */

#include "banker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Helper to free a 2D integer matrix (allocated as array of pointers) */
static void free_matrix(int **mat, int rows)
{
    if (!mat)
        return;
    for (int i = 0; i < rows; i++) {
        free(mat[i]);
    }
    free(mat);
}

/* Helper to allocate a 2D integer matrix */
static int **alloc_matrix(int rows, int cols)
{
    int **mat = safe_malloc((size_t)rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        mat[i] = safe_calloc((size_t)cols, sizeof(int));
    }
    return mat;
}

/*
 * banker_check_safety:
 * Checks whether the system is in a safe state and computes a safe execution sequence.
 *
 * Arguments:
 *   available: array of available resource instances [n_res]
 *   allocation: matrix of allocated resources [n_proc][n_res]
 *   maximum: matrix of maximum resource claims [n_proc][n_res]
 *   n_proc: number of processes
 *   n_res: number of resource types
 *   safe_sequence: output array of process indices in safe execution order [n_proc]
 *
 * Returns:
 *   true if system is in a safe state, false otherwise.
 */
bool banker_check_safety(int *available, int **allocation, int **maximum,
                         int n_proc, int n_res, int *safe_sequence)
{
    if (!available || !allocation || !maximum || !safe_sequence || n_proc <= 0 || n_res <= 0) {
        CC_ERROR("banker_check_safety: invalid arguments");
        return false;
    }

    /* Allocate Need matrix */
    int **need = alloc_matrix(n_proc, n_res);

    /* Calculate Need[i][j] = Maximum[i][j] - Allocation[i][j] and validate */
    for (int i = 0; i < n_proc; i++) {
        for (int j = 0; j < n_res; j++) {
            need[i][j] = maximum[i][j] - allocation[i][j];
            if (need[i][j] < 0) {
                CC_ERROR("Process P%d allocation (%d) exceeds maximum (%d) for resource R%d",
                         i, allocation[i][j], maximum[i][j], j);
                free_matrix(need, n_proc);
                return false;
            }
        }
    }

    /* Initialize Work[j] = Available[j] and Finish[i] = false */
    int *work = safe_malloc((size_t)n_res * sizeof(int));
    for (int j = 0; j < n_res; j++) {
        work[j] = available[j];
    }

    bool *finish = safe_calloc((size_t)n_proc, sizeof(bool));

    int count = 0;
    while (count < n_proc) {
        bool found = false;
        for (int i = 0; i < n_proc; i++) {
            if (!finish[i]) {
                bool can_satisfy = true;
                for (int j = 0; j < n_res; j++) {
                    if (need[i][j] > work[j]) {
                        can_satisfy = false;
                        break;
                    }
                }

                if (can_satisfy) {
                    for (int j = 0; j < n_res; j++) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    safe_sequence[count++] = i;
                    found = true;
                }
            }
        }

        /* If no process could be safely allocated resources in this pass, deadlock / unsafe */
        if (!found) {
            break;
        }
    }

    bool is_safe = (count == n_proc);

    free(work);
    free(finish);
    free_matrix(need, n_proc);

    return is_safe;
}

/*
 * banker_load_file:
 * Loads banker configuration from file:
 * Line 1: n_proc n_res
 * Line 2: Available[0] ... Available[n_res-1]
 * Next n_proc lines: Allocation matrix
 * Next n_proc lines: Maximum matrix
 */
bool banker_load_file(const char *path, int **available, int ***allocation,
                      int ***maximum, int *n_proc, int *n_res)
{
    if (!path || !available || !allocation || !maximum || !n_proc || !n_res) {
        CC_ERROR("banker_load_file: invalid arguments");
        return false;
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        CC_ERROR("Failed to open file: %s", path);
        return false;
    }

    int np = 0, nr = 0;
    if (fscanf(fp, "%d %d", &np, &nr) != 2) {
        CC_ERROR("Failed to read process and resource counts from %s", path);
        fclose(fp);
        return false;
    }

    if (np <= 0 || np > BANKER_MAX_PROCESSES || nr <= 0 || nr > BANKER_MAX_RESOURCES) {
        CC_ERROR("Invalid dimensions: %d processes, %d resources", np, nr);
        fclose(fp);
        return false;
    }

    int *avail = safe_malloc((size_t)nr * sizeof(int));
    for (int j = 0; j < nr; j++) {
        if (fscanf(fp, "%d", &avail[j]) != 1) {
            CC_ERROR("Failed to read Available resource %d", j);
            free(avail);
            fclose(fp);
            return false;
        }
        if (avail[j] < 0) {
            CC_ERROR("Negative Available resource value: %d", avail[j]);
            free(avail);
            fclose(fp);
            return false;
        }
    }

    int **alloc = alloc_matrix(np, nr);
    for (int i = 0; i < np; i++) {
        for (int j = 0; j < nr; j++) {
            if (fscanf(fp, "%d", &alloc[i][j]) != 1) {
                CC_ERROR("Failed to read Allocation[%d][%d]", i, j);
                free(avail);
                free_matrix(alloc, np);
                fclose(fp);
                return false;
            }
            if (alloc[i][j] < 0) {
                CC_ERROR("Negative Allocation value: %d", alloc[i][j]);
                free(avail);
                free_matrix(alloc, np);
                fclose(fp);
                return false;
            }
        }
    }

    int **max_mat = alloc_matrix(np, nr);
    for (int i = 0; i < np; i++) {
        for (int j = 0; j < nr; j++) {
            if (fscanf(fp, "%d", &max_mat[i][j]) != 1) {
                CC_ERROR("Failed to read Maximum[%d][%d]", i, j);
                free(avail);
                free_matrix(alloc, np);
                free_matrix(max_mat, np);
                fclose(fp);
                return false;
            }
            if (max_mat[i][j] < 0) {
                CC_ERROR("Negative Maximum value: %d", max_mat[i][j]);
                free(avail);
                free_matrix(alloc, np);
                free_matrix(max_mat, np);
                fclose(fp);
                return false;
            }
            if (alloc[i][j] > max_mat[i][j]) {
                CC_ERROR("Allocation[%d][%d] (%d) exceeds Maximum (%d)",
                         i, j, alloc[i][j], max_mat[i][j]);
                free(avail);
                free_matrix(alloc, np);
                free_matrix(max_mat, np);
                fclose(fp);
                return false;
            }
        }
    }

    fclose(fp);

    *available = avail;
    *allocation = alloc;
    *maximum = max_mat;
    *n_proc = np;
    *n_res = nr;

    return true;
}

/* Print state helper implementation */
void banker_print_state(const BankerState *state)
{
    if (!state)
        return;

    printf("\n--- Banker's State ---\n");
    printf("Processes: %d, Resource types: %d\n", state->num_processes, state->num_resources);

    printf("Available: [");
    for (int j = 0; j < state->num_resources; j++) {
        printf(" %d", state->available[j]);
    }
    printf(" ]\n\n");

    printf("%-8s | %-15s | %-15s | %-15s\n", "Process", "Allocation", "Maximum", "Need");
    printf("---------+-----------------+-----------------+-----------------\n");

    for (int i = 0; i < state->num_processes; i++) {
        printf("P%-7d | ", i);
        for (int j = 0; j < state->num_resources; j++) {
            printf("%2d ", state->allocation[i][j]);
        }
        printf("    | ");
        for (int j = 0; j < state->num_resources; j++) {
            printf("%2d ", state->max[i][j]);
        }
        printf("    | ");
        for (int j = 0; j < state->num_resources; j++) {
            printf("%2d ", state->need[i][j]);
        }
        printf("\n");
    }
}

/* Helper to print matrix tables with explicit Need = Maximum - Allocation */
static void print_matrices_formatted(int *available, int **allocation, int **maximum,
                                     int **need, int n_proc, int n_res)
{
    print_separator();
    printf("Available Resources Vector:\n  ");
    for (int j = 0; j < n_res; j++) {
        printf("R%-2d ", j);
    }
    printf("\n  ");
    for (int j = 0; j < n_res; j++) {
        printf("%-3d ", available[j]);
    }
    printf("\n\n");

    printf("Resource Allocation and Need Table (Need = Maximum - Allocation):\n");
    printf("%-8s | ", "Process");
    for (int j = 0; j < n_res; j++) printf("R%d ", j);
    printf(" (Alloc) | ");
    for (int j = 0; j < n_res; j++) printf("R%d ", j);
    printf(" (Max)   | ");
    for (int j = 0; j < n_res; j++) printf("R%d ", j);
    printf(" (Need)\n");

    print_separator();

    for (int i = 0; i < n_proc; i++) {
        printf("P%-7d | ", i);
        for (int j = 0; j < n_res; j++) {
            printf("%-2d ", allocation[i][j]);
        }
        printf("        | ");
        for (int j = 0; j < n_res; j++) {
            printf("%-2d ", maximum[i][j]);
        }
        printf("      | ");
        for (int j = 0; j < n_res; j++) {
            printf("%-2d ", need[i][j]);
        }
        printf("\n");
    }
    print_separator();
}

/*
 * banker_run_safe:
 * Loads data/banker_safe.txt, computes Need, runs safety check, prints safe sequence.
 */
int banker_run_safe(void)
{
    print_section("Banker's Safety Algorithm Demo: Safe State");

    const char *path = "data/banker_safe.txt";
    int *available = NULL;
    int **allocation = NULL;
    int **maximum = NULL;
    int n_proc = 0, n_res = 0;

    if (!banker_load_file(path, &available, &allocation, &maximum, &n_proc, &n_res)) {
        CC_ERROR("Failed to load %s", path);
        return 1;
    }

    int **need = alloc_matrix(n_proc, n_res);
    for (int i = 0; i < n_proc; i++) {
        for (int j = 0; j < n_res; j++) {
            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }

    print_matrices_formatted(available, allocation, maximum, need, n_proc, n_res);

    int *safe_seq = safe_malloc((size_t)n_proc * sizeof(int));
    bool safe = banker_check_safety(available, allocation, maximum, n_proc, n_res, safe_seq);

    if (safe) {
        printf("\nSystem Status: SAFE STATE\n");
        printf("Safe execution sequence: ");
        for (int i = 0; i < n_proc; i++) {
            printf("P%d%s", safe_seq[i], (i == n_proc - 1) ? "" : " -> ");
        }
        printf("\n\nAll processes can finish without entering deadlock.\n");
    } else {
        printf("\nSystem Status: UNSAFE STATE\n");
    }

    free(safe_seq);
    free(available);
    free_matrix(allocation, n_proc);
    free_matrix(maximum, n_proc);
    free_matrix(need, n_proc);

    return safe ? 0 : 1;
}

/*
 * banker_run_unsafe:
 * Loads data/banker_unsafe.txt, computes Need, runs safety check, explains unsatisfied processes.
 */
int banker_run_unsafe(void)
{
    print_section("Banker's Safety Algorithm Demo: Unsafe State");

    const char *path = "data/banker_unsafe.txt";
    int *available = NULL;
    int **allocation = NULL;
    int **maximum = NULL;
    int n_proc = 0, n_res = 0;

    if (!banker_load_file(path, &available, &allocation, &maximum, &n_proc, &n_res)) {
        CC_ERROR("Failed to load %s", path);
        return 1;
    }

    int **need = alloc_matrix(n_proc, n_res);
    for (int i = 0; i < n_proc; i++) {
        for (int j = 0; j < n_res; j++) {
            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }

    print_matrices_formatted(available, allocation, maximum, need, n_proc, n_res);

    int *safe_seq = safe_malloc((size_t)n_proc * sizeof(int));
    bool safe = banker_check_safety(available, allocation, maximum, n_proc, n_res, safe_seq);

    if (safe) {
        printf("\nSystem Status: SAFE STATE\n");
    } else {
        printf("\nSystem Status: UNSAFE STATE - No safe sequence exists\n");
        printf("\nDetailed Analysis of Blocked Processes:\n");

        /* Track Work and Finish to show which processes were blocked and why */
        int *work = safe_malloc((size_t)n_res * sizeof(int));
        for (int j = 0; j < n_res; j++) {
            work[j] = available[j];
        }
        bool *finish = safe_calloc((size_t)n_proc, sizeof(bool));

        /* Run one simulation pass to find processes that could complete, if any */
        bool found = true;
        while (found) {
            found = false;
            for (int i = 0; i < n_proc; i++) {
                if (!finish[i]) {
                    bool can_satisfy = true;
                    for (int j = 0; j < n_res; j++) {
                        if (need[i][j] > work[j]) {
                            can_satisfy = false;
                            break;
                        }
                    }
                    if (can_satisfy) {
                        for (int j = 0; j < n_res; j++) {
                            work[j] += allocation[i][j];
                        }
                        finish[i] = true;
                        found = true;
                    }
                }
            }
        }

        /* Explain unsatisfied processes */
        for (int i = 0; i < n_proc; i++) {
            if (!finish[i]) {
                printf("  - Process P%d could not proceed: Need [", i);
                for (int j = 0; j < n_res; j++) {
                    printf(" %d", need[i][j]);
                }
                printf(" ] exceeds Work/Available [");
                for (int j = 0; j < n_res; j++) {
                    printf(" %d", work[j]);
                }
                printf(" ]\n");
            }
        }
        printf("Potential circular wait or resource starvation detected. Safe execution impossible.\n");

        free(work);
        free(finish);
    }

    free(safe_seq);
    free(available);
    free_matrix(allocation, n_proc);
    free_matrix(maximum, n_proc);
    free_matrix(need, n_proc);

    /* Return 0 indicating the unsafe state demo executed successfully and verified unsafe state */
    return 0;
}
