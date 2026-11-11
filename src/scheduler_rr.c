/*
 * scheduler_rr.c - Round-Robin scheduling implementation
 *
 * Implements preemptive Round-Robin scheduling using an array-based
 * circular ready queue. New arrivals during a time slice are enqueued
 * before requeueing the current job. Handles idle periods.
 */

#include "scheduler.h"
#include "jobs.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

typedef struct {
    int orig_index;
    int64_t arrival_time;
} RRArrivalEntry;

static int compare_rr_arrivals(const void *a, const void *b)
{
    const RRArrivalEntry *ea = (const RRArrivalEntry *)a;
    const RRArrivalEntry *eb = (const RRArrivalEntry *)b;

    if (ea->arrival_time < eb->arrival_time) {
        return -1;
    }
    if (ea->arrival_time > eb->arrival_time) {
        return 1;
    }
    if (ea->orig_index < eb->orig_index) {
        return -1;
    }
    if (ea->orig_index > eb->orig_index) {
        return 1;
    }
    return 0;
}

ScheduleResult schedule_rr(const JobList *jobs, int quantum)
{
    ScheduleResult res;
    memset(&res, 0, sizeof(res));

    if (quantum <= 0) {
        CC_ERROR("schedule_rr: invalid quantum %d (must be > 0)", quantum);
        schedule_result_init(&res, 0);
        return res;
    }

    if (!jobs || jobs->count == 0) {
        schedule_result_init(&res, 0);
        return res;
    }

    /* Work on an isolated copy of jobs */
    JobList *copy = job_list_copy(jobs);
    if (!copy) {
        CC_ERROR("schedule_rr: failed to copy jobs");
        schedule_result_init(&res, 0);
        return res;
    }

    int n = copy->count;
    if (!schedule_result_init(&res, n)) {
        job_list_free(copy);
        return res;
    }

    /* Record job_ids in original order */
    for (int i = 0; i < n; i++) {
        res.job_ids[i] = copy->jobs[i].job_id;
        /* Ensure remaining time is set to burst time */
        copy->jobs[i].remaining_time = copy->jobs[i].burst_time;
    }

    /* Sort arrival order with stable tie-breaking */
    RRArrivalEntry *arr = (RRArrivalEntry *)safe_malloc((size_t)n * sizeof(RRArrivalEntry));
    for (int i = 0; i < n; i++) {
        arr[i].orig_index = i;
        arr[i].arrival_time = copy->jobs[i].arrival_time;
    }
    qsort(arr, (size_t)n, sizeof(RRArrivalEntry), compare_rr_arrivals);

    /* Array-based circular ready queue */
    int q_capacity = n + 4;
    int *queue = (int *)safe_malloc((size_t)q_capacity * sizeof(int));
    int q_head = 0;
    int q_tail = 0;
    int q_count = 0;

    int next_arrival = 0;
    int completed = 0;
    int64_t current_time = 0;
    double total_tat = 0.0;
    double total_wt = 0.0;

    while (completed < n) {
        /* If ready queue is empty, advance to next job arrival or enqueue arrived jobs */
        if (q_count == 0) {
            if (next_arrival < n) {
                if (arr[next_arrival].arrival_time > current_time) {
                    /* CPU is idle until next job arrives */
                    schedule_result_add_segment(&res, -1, current_time, arr[next_arrival].arrival_time);
                    current_time = arr[next_arrival].arrival_time;
                }
                while (next_arrival < n && arr[next_arrival].arrival_time <= current_time) {
                    queue[q_tail] = arr[next_arrival].orig_index;
                    q_tail = (q_tail + 1) % q_capacity;
                    q_count++;
                    next_arrival++;
                }
            } else {
                break;
            }
        }

        /* Dequeue the next ready job */
        int cur_idx = queue[q_head];
        q_head = (q_head + 1) % q_capacity;
        q_count--;

        Job *j = &copy->jobs[cur_idx];
        int64_t slice = (j->remaining_time < (int64_t)quantum) ? j->remaining_time : (int64_t)quantum;
        int64_t start_time = current_time;
        int64_t end_time = current_time + slice;

        schedule_result_add_segment(&res, j->job_id, start_time, end_time);
        j->remaining_time -= slice;
        current_time = end_time;

        /* Enqueue any new arrivals during this slice BEFORE requeuing the current job */
        while (next_arrival < n && arr[next_arrival].arrival_time <= current_time) {
            queue[q_tail] = arr[next_arrival].orig_index;
            q_tail = (q_tail + 1) % q_capacity;
            q_count++;
            next_arrival++;
        }

        if (j->remaining_time > 0) {
            /* Re-enqueue the unfinished job */
            queue[q_tail] = cur_idx;
            q_tail = (q_tail + 1) % q_capacity;
            q_count++;
        } else {
            /* Job finished execution */
            completed++;
            int64_t ct = current_time;
            int64_t tat = ct - j->arrival_time;
            int64_t wt = tat - j->burst_time;

            res.completion_times[cur_idx] = ct;
            res.turnaround_times[cur_idx] = tat;
            res.waiting_times[cur_idx] = wt;

            total_tat += (double)tat;
            total_wt += (double)wt;
        }
    }

    res.avg_turnaround = total_tat / (double)n;
    res.avg_waiting = total_wt / (double)n;

    free(queue);
    free(arr);
    job_list_free(copy);

    return res;
}
