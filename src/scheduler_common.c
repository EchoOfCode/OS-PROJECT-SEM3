/*
 * scheduler_common.c - Shared data structures and utilities for schedulers
 *
 * Implements ScheduleResult lifecycle, timeline segment management,
 * formatted metric table printing, ASCII Gantt chart rendering, and
 * algorithm comparison.
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

#define SEGMENT_INITIAL_CAPACITY 16
#define GANTT_MAX_SEGMENTS_PER_LINE 10

/* ------------------------------------------------------------------ */
/*  ScheduleResult Lifecycle                                          */
/* ------------------------------------------------------------------ */

bool schedule_result_init(ScheduleResult *r, int job_count)
{
    if (!r) {
        CC_ERROR("schedule_result_init: NULL result pointer");
        return false;
    }

    if (job_count < 0) {
        CC_ERROR("schedule_result_init: negative job_count %d", job_count);
        return false;
    }

    r->job_count = job_count;
    r->segment_count = 0;
    r->segment_capacity = (job_count > 0) ? (job_count * 4) : SEGMENT_INITIAL_CAPACITY;
    if (r->segment_capacity < SEGMENT_INITIAL_CAPACITY) {
        r->segment_capacity = SEGMENT_INITIAL_CAPACITY;
    }

    r->segments = (TimelineSegment *)safe_calloc((size_t)r->segment_capacity, sizeof(TimelineSegment));

    if (job_count > 0) {
        r->completion_times = (int64_t *)safe_calloc((size_t)job_count, sizeof(int64_t));
        r->turnaround_times = (int64_t *)safe_calloc((size_t)job_count, sizeof(int64_t));
        r->waiting_times = (int64_t *)safe_calloc((size_t)job_count, sizeof(int64_t));
        r->job_ids = (int *)safe_calloc((size_t)job_count, sizeof(int));
    } else {
        r->completion_times = NULL;
        r->turnaround_times = NULL;
        r->waiting_times = NULL;
        r->job_ids = NULL;
    }

    r->avg_turnaround = 0.0;
    r->avg_waiting = 0.0;

    return true;
}

void schedule_result_free(ScheduleResult *r)
{
    if (!r) {
        return;
    }

    if (r->segments) {
        free(r->segments);
        r->segments = NULL;
    }
    if (r->completion_times) {
        free(r->completion_times);
        r->completion_times = NULL;
    }
    if (r->turnaround_times) {
        free(r->turnaround_times);
        r->turnaround_times = NULL;
    }
    if (r->waiting_times) {
        free(r->waiting_times);
        r->waiting_times = NULL;
    }
    if (r->job_ids) {
        free(r->job_ids);
        r->job_ids = NULL;
    }

    r->segment_count = 0;
    r->segment_capacity = 0;
    r->job_count = 0;
    r->avg_turnaround = 0.0;
    r->avg_waiting = 0.0;
}

bool schedule_result_add_segment(ScheduleResult *r, int job_id, int64_t start, int64_t end)
{
    if (!r) {
        CC_ERROR("schedule_result_add_segment: NULL result pointer");
        return false;
    }

    if (start >= end) {
        return false;
    }

    if (r->segment_count >= r->segment_capacity) {
        int new_capacity = (r->segment_capacity > 0) ? (r->segment_capacity * 2) : SEGMENT_INITIAL_CAPACITY;
        r->segments = (TimelineSegment *)safe_realloc(r->segments, (size_t)new_capacity * sizeof(TimelineSegment));
        r->segment_capacity = new_capacity;
    }

    r->segments[r->segment_count].job_id = job_id;
    r->segments[r->segment_count].start_time = start;
    r->segments[r->segment_count].end_time = end;
    r->segment_count++;

    return true;
}

/* ------------------------------------------------------------------ */
/*  Formatted Output                                                  */
/* ------------------------------------------------------------------ */

static const Job *find_job_by_id(const JobList *jobs, int job_id)
{
    if (!jobs || !jobs->jobs) {
        return NULL;
    }
    for (int i = 0; i < jobs->count; i++) {
        if (jobs->jobs[i].job_id == job_id) {
            return &jobs->jobs[i];
        }
    }
    return NULL;
}

void schedule_print_result(const char *name, const ScheduleResult *r, const JobList *jobs)
{
    if (!r) {
        CC_ERROR("schedule_print_result: NULL result");
        return;
    }

    char title_buf[128];
    if (name && name[0] != '\0') {
        snprintf(title_buf, sizeof(title_buf), "%s Scheduling Results", name);
    } else {
        snprintf(title_buf, sizeof(title_buf), "Scheduling Results");
    }
    print_section(title_buf);

    if (r->job_count == 0) {
        printf("No jobs scheduled.\n");
        print_separator();
        return;
    }

    if (jobs && jobs->count > 0) {
        printf(" %-8s | %-12s | %-10s | %-8s | %-15s | %-15s | %-12s\n",
               "Job ID", "Arrival Time", "Burst Time", "Priority",
               "Completion Time", "Turnaround Time", "Waiting Time");
        print_separator();

        for (int i = 0; i < r->job_count; i++) {
            int jid = r->job_ids[i];
            const Job *j = find_job_by_id(jobs, jid);
            if (j) {
                printf(" %-8d | %-12" PRId64 " | %-10" PRId64 " | %-8d | %-15" PRId64 " | %-15" PRId64 " | %-12" PRId64 "\n",
                       jid, j->arrival_time, j->burst_time, j->priority,
                       r->completion_times[i], r->turnaround_times[i], r->waiting_times[i]);
            } else {
                printf(" %-8d | %-12s | %-10s | %-8s | %-15" PRId64 " | %-15" PRId64 " | %-12" PRId64 "\n",
                       jid, "-", "-", "-",
                       r->completion_times[i], r->turnaround_times[i], r->waiting_times[i]);
            }
        }
    } else {
        printf(" %-8s | %-15s | %-15s | %-12s\n",
               "Job ID", "Completion Time", "Turnaround Time", "Waiting Time");
        print_separator();

        for (int i = 0; i < r->job_count; i++) {
            printf(" %-8d | %-15" PRId64 " | %-15" PRId64 " | %-12" PRId64 "\n",
                   r->job_ids[i],
                   r->completion_times[i], r->turnaround_times[i], r->waiting_times[i]);
        }
    }

    print_separator();
    printf(" Average Turnaround Time: %.2f\n", r->avg_turnaround);
    printf(" Average Waiting Time:    %.2f\n", r->avg_waiting);
    print_separator();
}

void schedule_print_gantt(const ScheduleResult *r)
{
    if (!r) {
        CC_ERROR("schedule_print_gantt: NULL result");
        return;
    }

    print_section("Gantt Chart");

    if (r->segment_count == 0) {
        printf("  (empty timeline)\n\n");
        return;
    }

    int seg_idx = 0;
    while (seg_idx < r->segment_count) {
        int chunk_end = seg_idx + GANTT_MAX_SEGMENTS_PER_LINE;
        if (chunk_end > r->segment_count) {
            chunk_end = r->segment_count;
        }

        char bar[1024];
        char timeline[1024];
        memset(bar, ' ', sizeof(bar));
        memset(timeline, ' ', sizeof(timeline));

        int bar_pos = 2;

        for (int i = seg_idx; i < chunk_end; i++) {
            const TimelineSegment *seg = &r->segments[i];

            /* Left boundary '|' */
            bar[bar_pos] = '|';

            /* Place start time on timeline under '|' if at start of chunk */
            if (i == seg_idx) {
                char t_buf[32];
                int t_len = snprintf(t_buf, sizeof(t_buf), "%" PRId64, seg->start_time);
                for (int c = 0; c < t_len && (bar_pos + c) < (int)sizeof(timeline); c++) {
                    timeline[bar_pos + c] = t_buf[c];
                }
            }

            /* Format label */
            char label[32];
            if (seg->job_id <= 0) {
                snprintf(label, sizeof(label), "IDLE");
            } else {
                snprintf(label, sizeof(label), "J%d", seg->job_id);
            }

            int label_len = (int)strlen(label);
            int cell_width = (label_len < 4) ? 6 : (label_len + 2);
            int left_pad = (cell_width - label_len) / 2;

            /* Copy label centered */
            for (int c = 0; c < label_len; c++) {
                bar[bar_pos + 1 + left_pad + c] = label[c];
            }

            bar_pos += 1 + cell_width;

            /* Place end time under this '|' */
            char end_buf[32];
            int end_len = snprintf(end_buf, sizeof(end_buf), "%" PRId64, seg->end_time);
            for (int c = 0; c < end_len && (bar_pos + c) < (int)sizeof(timeline); c++) {
                timeline[bar_pos + c] = end_buf[c];
            }
        }

        /* Closing boundary */
        bar[bar_pos] = '|';
        bar[bar_pos + 1] = '\0';

        /* Trim timeline line trailing spaces */
        int tl_len = (int)sizeof(timeline) - 1;
        while (tl_len >= 0 && timeline[tl_len] == ' ') {
            tl_len--;
        }
        timeline[tl_len + 1] = '\0';

        printf("%s\n", bar);
        printf("%s\n\n", timeline);

        seg_idx = chunk_end;
    }
}

/* ------------------------------------------------------------------ */
/*  Comparison                                                        */
/* ------------------------------------------------------------------ */

void schedule_compare_all(const JobList *jobs, int quantum)
{
    if (!jobs || jobs->count == 0) {
        CC_WARN("schedule_compare_all: empty job list");
        return;
    }

    /* 1. Run FCFS */
    ScheduleResult r_fcfs = schedule_fcfs(jobs);
    schedule_print_result("FCFS", &r_fcfs, jobs);
    schedule_print_gantt(&r_fcfs);

    /* 2. Run Round-Robin */
    char rr_title[64];
    snprintf(rr_title, sizeof(rr_title), "Round-Robin (q=%d)", quantum);
    ScheduleResult r_rr = schedule_rr(jobs, quantum);
    schedule_print_result(rr_title, &r_rr, jobs);
    schedule_print_gantt(&r_rr);

    /* 3. Run Priority */
    ScheduleResult r_prio = schedule_priority(jobs);
    schedule_print_result("Priority (Non-Preemptive)", &r_prio, jobs);
    schedule_print_gantt(&r_prio);

    /* 4. Comparison Table */
    print_section("Scheduler Comparison Summary");
    printf(" %-30s | %-19s | %-16s\n",
           "Algorithm", "Avg Turnaround Time", "Avg Waiting Time");
    print_separator();
    printf(" %-30s | %19.2f | %16.2f\n",
           "FCFS", r_fcfs.avg_turnaround, r_fcfs.avg_waiting);
    printf(" %-30s | %19.2f | %16.2f\n",
           rr_title, r_rr.avg_turnaround, r_rr.avg_waiting);
    printf(" %-30s | %19.2f | %16.2f\n",
           "Priority (Non-Preemptive)", r_prio.avg_turnaround, r_prio.avg_waiting);
    print_separator();

    /* 5. Free results */
    schedule_result_free(&r_fcfs);
    schedule_result_free(&r_rr);
    schedule_result_free(&r_prio);
}
