/*
 * scheduler_priority.c - Non-preemptive Priority scheduling implementation
 *
 * Jobs are scheduled by priority (lowest integer value = highest priority).
 * Ties are broken by earlier arrival time, then original input order.
 * Once scheduled, each job runs to completion. Handles idle periods.
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

ScheduleResult schedule_priority(const JobList *jobs)
{
    ScheduleResult res;
    memset(&res, 0, sizeof(res));

    if (!jobs || jobs->count == 0) {
        schedule_result_init(&res, 0);
        return res;
    }

    /* Work on an isolated copy of jobs */
    JobList *copy = job_list_copy(jobs);
    if (!copy) {
        CC_ERROR("schedule_priority: failed to copy jobs");
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
    }

    bool *completed = (bool *)safe_calloc((size_t)n, sizeof(bool));
    int completed_count = 0;
    int64_t current_time = 0;
    double total_tat = 0.0;
    double total_wt = 0.0;

    while (completed_count < n) {
        int best_idx = -1;

        /* Find the highest priority job that has arrived */
        for (int i = 0; i < n; i++) {
            if (completed[i]) {
                continue;
            }

            const Job *candidate = &copy->jobs[i];
            if (candidate->arrival_time <= current_time) {
                if (best_idx == -1) {
                    best_idx = i;
                } else {
                    const Job *best = &copy->jobs[best_idx];
                    if (candidate->priority < best->priority) {
                        best_idx = i;
                    } else if (candidate->priority == best->priority) {
                        if (candidate->arrival_time < best->arrival_time) {
                            best_idx = i;
                        }
                        /* Ties in arrival_time are preserved by earlier index i < best_idx */
                    }
                }
            }
        }

        /* If no arrived job is ready, CPU is idle until the next earliest arrival */
        if (best_idx == -1) {
            int64_t next_arrival = INT64_MAX;
            for (int i = 0; i < n; i++) {
                if (!completed[i] && copy->jobs[i].arrival_time < next_arrival) {
                    next_arrival = copy->jobs[i].arrival_time;
                }
            }

            if (next_arrival > current_time) {
                schedule_result_add_segment(&res, -1, current_time, next_arrival);
                current_time = next_arrival;
            }
            continue;
        }

        /* Execute the selected job to completion */
        Job *j = &copy->jobs[best_idx];
        int64_t start_time = current_time;
        int64_t end_time = current_time + j->burst_time;

        schedule_result_add_segment(&res, j->job_id, start_time, end_time);
        current_time = end_time;

        int64_t ct = current_time;
        int64_t tat = ct - j->arrival_time;
        int64_t wt = tat - j->burst_time;

        res.completion_times[best_idx] = ct;
        res.turnaround_times[best_idx] = tat;
        res.waiting_times[best_idx] = wt;

        completed[best_idx] = true;
        completed_count++;

        total_tat += (double)tat;
        total_wt += (double)wt;
    }

    res.avg_turnaround = total_tat / (double)n;
    res.avg_waiting = total_wt / (double)n;

    free(completed);
    job_list_free(copy);

    return res;
}
