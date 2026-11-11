/*
 * scheduler_fcfs.c - First-Come, First-Served scheduling implementation
 *
 * Jobs are scheduled strictly in order of their arrival time. Ties in arrival
 * time are broken by original input order. Handles idle CPU gaps.
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
    Job job;
    int orig_index;
} FCFSEntry;

static int compare_fcfs_entries(const void *a, const void *b)
{
    const FCFSEntry *ea = (const FCFSEntry *)a;
    const FCFSEntry *eb = (const FCFSEntry *)b;

    if (ea->job.arrival_time < eb->job.arrival_time) {
        return -1;
    }
    if (ea->job.arrival_time > eb->job.arrival_time) {
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

ScheduleResult schedule_fcfs(const JobList *jobs)
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
        CC_ERROR("schedule_fcfs: failed to copy jobs");
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

    /* Prepare sorting entries to maintain tie-breaking by original index */
    FCFSEntry *entries = (FCFSEntry *)safe_malloc((size_t)n * sizeof(FCFSEntry));
    for (int i = 0; i < n; i++) {
        entries[i].job = copy->jobs[i];
        entries[i].orig_index = i;
    }

    qsort(entries, (size_t)n, sizeof(FCFSEntry), compare_fcfs_entries);

    int64_t current_time = 0;
    double total_tat = 0.0;
    double total_wt = 0.0;

    for (int i = 0; i < n; i++) {
        const Job *j = &entries[i].job;
        int target_idx = entries[i].orig_index;

        /* If CPU is idle before this job arrives, record IDLE segment */
        if (j->arrival_time > current_time) {
            schedule_result_add_segment(&res, -1, current_time, j->arrival_time);
            current_time = j->arrival_time;
        }

        int64_t start_time = current_time;
        int64_t end_time = current_time + j->burst_time;

        schedule_result_add_segment(&res, j->job_id, start_time, end_time);
        current_time = end_time;

        int64_t ct = current_time;
        int64_t tat = ct - j->arrival_time;
        int64_t wt = tat - j->burst_time;

        res.completion_times[target_idx] = ct;
        res.turnaround_times[target_idx] = tat;
        res.waiting_times[target_idx] = wt;

        total_tat += (double)tat;
        total_wt += (double)wt;
    }

    res.avg_turnaround = total_tat / (double)n;
    res.avg_waiting = total_wt / (double)n;

    free(entries);
    job_list_free(copy);

    return res;
}
