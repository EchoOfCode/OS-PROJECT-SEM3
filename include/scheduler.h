/*
 * scheduler.h - CPU scheduling algorithms for CampusCompute
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"
#include "jobs.h"

/* One segment of a Gantt-chart timeline */
typedef struct {
    int job_id;
    int64_t start_time;
    int64_t end_time;
} TimelineSegment;

/* Full result of a scheduling run */
typedef struct {
    TimelineSegment *segments;
    int segment_count;
    int segment_capacity;

    int64_t *completion_times;
    int64_t *turnaround_times;
    int64_t *waiting_times;

    double avg_turnaround;
    double avg_waiting;

    int job_count;
    int *job_ids;
} ScheduleResult;

/* Functions in src/scheduler_common.c */
bool schedule_result_init(ScheduleResult *r, int job_count);
void schedule_result_free(ScheduleResult *r);
bool schedule_result_add_segment(ScheduleResult *r, int job_id, int64_t start, int64_t end);
void schedule_print_result(const char *name, const ScheduleResult *r, const JobList *jobs);
void schedule_print_gantt(const ScheduleResult *r);
void schedule_compare_all(const JobList *jobs, int quantum);

/* Schedulers */
ScheduleResult schedule_fcfs(const JobList *jobs);
ScheduleResult schedule_rr(const JobList *jobs, int quantum);
ScheduleResult schedule_priority(const JobList *jobs);

#endif /* SCHEDULER_H */
