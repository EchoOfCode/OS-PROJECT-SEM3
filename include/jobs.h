/*
 * jobs.h - Job structures and management for CampusCompute
 */
#ifndef JOBS_H
#define JOBS_H

#include "common.h"

/* Job lifecycle states */
typedef enum {
    JOB_NEW,
    JOB_READY,
    JOB_RUNNING,
    JOB_COMPLETED
} JobStatus;

/* A single schedulable job */
typedef struct {
    int job_id;
    int64_t arrival_time;
    int64_t burst_time;
    int64_t remaining_time;
    int priority;
    int64_t completion_time;
    int64_t turnaround_time;
    int64_t waiting_time;
    JobStatus status;
} Job;

/* Dynamic array of jobs */
typedef struct {
    Job *jobs;
    int count;
    int capacity;
} JobList;

/* Job management functions */
JobList *job_list_create(int capacity);
void job_list_free(JobList *list);
bool job_list_add(JobList *list, const Job *job);
JobList *job_list_load_csv(const char *path);
JobList *job_list_copy(const JobList *src);
void job_list_print(const JobList *list);
bool job_validate(const Job *job);

#endif /* JOBS_H */
