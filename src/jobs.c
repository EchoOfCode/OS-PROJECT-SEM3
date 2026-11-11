/*
 * jobs.c - Job management implementation for CampusCompute
 *
 * Implements job loading from CSV, validation, storage in dynamic JobList,
 * copying for scheduler isolation, and formatted printing.
 */

#include "jobs.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/*  Validation                                                        */
/* ------------------------------------------------------------------ */

bool job_validate(const Job *job)
{
    if (!job) {
        return false;
    }

    if (job->burst_time <= 0) {
        return false;
    }

    if (job->arrival_time < 0) {
        return false;
    }

    if (job->priority < 1 || job->priority > 10) {
        return false;
    }

    return true;
}

/* ------------------------------------------------------------------ */
/*  Lifecycle                                                         */
/* ------------------------------------------------------------------ */

JobList *job_list_create(int initial_capacity)
{
    if (initial_capacity <= 0) {
        initial_capacity = 8;
    }

    JobList *list = (JobList *)safe_malloc(sizeof(JobList));
    list->jobs = (Job *)safe_malloc(sizeof(Job) * (size_t)initial_capacity);
    list->count = 0;
    list->capacity = initial_capacity;

    return list;
}

void job_list_free(JobList *list)
{
    if (!list) {
        return;
    }

    if (list->jobs) {
        free(list->jobs);
        list->jobs = NULL;
    }
    list->count = 0;
    list->capacity = 0;
    free(list);
}

/* ------------------------------------------------------------------ */
/*  Operations                                                        */
/* ------------------------------------------------------------------ */

bool job_list_add(JobList *list, const Job *job)
{
    if (!list || !job) {
        CC_ERROR("job_list_add: NULL list or job");
        return false;
    }

    if (!job_validate(job)) {
        CC_ERROR("job_list_add: validation failed for job_id %d", job->job_id);
        return false;
    }

    /* Reject duplicate job IDs */
    for (int i = 0; i < list->count; i++) {
        if (list->jobs[i].job_id == job->job_id) {
            CC_ERROR("job_list_add: duplicate job_id %d", job->job_id);
            return false;
        }
    }

    /* Expand capacity if necessary */
    if (list->count >= list->capacity) {
        int new_capacity = (list->capacity > 0) ? list->capacity * 2 : 8;
        Job *new_jobs = (Job *)safe_realloc(list->jobs, sizeof(Job) * (size_t)new_capacity);
        list->jobs = new_jobs;
        list->capacity = new_capacity;
    }

    /* Insert job and ensure default state */
    Job *dest = &list->jobs[list->count];
    *dest = *job;
    if (dest->remaining_time <= 0) {
        dest->remaining_time = job->burst_time;
    }
    dest->completion_time = 0;
    dest->turnaround_time = 0;
    dest->waiting_time = 0;
    dest->status = JOB_NEW;

    list->count++;
    return true;
}

JobList *job_list_copy(const JobList *src)
{
    if (!src) {
        return NULL;
    }

    int cap = (src->capacity > 0) ? src->capacity : (src->count > 0 ? src->count : 8);
    JobList *copy = job_list_create(cap);
    if (!copy) {
        return NULL;
    }

    if (src->count > 0 && src->jobs) {
        memcpy(copy->jobs, src->jobs, sizeof(Job) * (size_t)src->count);
    }
    copy->count = src->count;

    return copy;
}

JobList *job_list_load_csv(const char *filename)
{
    if (!filename) {
        CC_ERROR("job_list_load_csv: NULL filename provided");
        return NULL;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        CC_ERROR("job_list_load_csv: failed to open '%s': %s", filename, strerror(errno));
        return NULL;
    }

    JobList *list = job_list_create(16);
    if (!list) {
        fclose(fp);
        return NULL;
    }

    char line[512];
    int line_number = 0;
    bool header_skipped = false;

    while (fgets(line, sizeof(line), fp)) {
        line_number++;

        /* Strip trailing newline / carriage return */
        char *p = line;
        while (*p) {
            if (*p == '\r' || *p == '\n') {
                *p = '\0';
                break;
            }
            p++;
        }

        /* Check for blank line */
        char *trimmed = str_trim(line);
        if (*trimmed == '\0') {
            continue;
        }

        /* Skip header line */
        if (!header_skipped) {
            header_skipped = true;
            continue;
        }

        /* Split by comma into 4 tokens: job_id,arrival_time,burst_time,priority */
        char *comma1 = strchr(trimmed, ',');
        if (!comma1) {
            CC_ERROR("Line %d: parse error: missing comma after job_id in '%s'", line_number, line);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }
        *comma1 = '\0';

        char *token2 = comma1 + 1;
        char *comma2 = strchr(token2, ',');
        if (!comma2) {
            CC_ERROR("Line %d: parse error: missing comma after arrival_time in '%s'", line_number, line);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }
        *comma2 = '\0';

        char *token3 = comma2 + 1;
        char *comma3 = strchr(token3, ',');
        if (!comma3) {
            CC_ERROR("Line %d: parse error: missing comma after burst_time in '%s'", line_number, line);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }
        *comma3 = '\0';

        char *token4 = comma3 + 1;
        if (strchr(token4, ',') != NULL) {
            CC_ERROR("Line %d: parse error: unexpected extra column in '%s'", line_number, line);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }

        char *s_id = str_trim(trimmed);
        char *s_arr = str_trim(token2);
        char *s_burst = str_trim(token3);
        char *s_prio = str_trim(token4);

        if (*s_id == '\0' || *s_arr == '\0' || *s_burst == '\0' || *s_prio == '\0') {
            CC_ERROR("Line %d: parse error: empty column value", line_number);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }

        long job_id = 0;
        long long arrival_time = 0;
        long long burst_time = 0;
        long priority = 0;

        if (!safe_strtol(s_id, &job_id)) {
            CC_ERROR("Line %d: parse error: invalid job_id '%s'", line_number, s_id);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }

        if (!safe_strtoll(s_arr, &arrival_time)) {
            CC_ERROR("Line %d: parse error: invalid arrival_time '%s'", line_number, s_arr);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }

        if (!safe_strtoll(s_burst, &burst_time)) {
            CC_ERROR("Line %d: parse error: invalid burst_time '%s'", line_number, s_burst);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }

        if (!safe_strtol(s_prio, &priority)) {
            CC_ERROR("Line %d: parse error: invalid priority '%s'", line_number, s_prio);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }

        Job job;
        memset(&job, 0, sizeof(Job));
        job.job_id = (int)job_id;
        job.arrival_time = (int64_t)arrival_time;
        job.burst_time = (int64_t)burst_time;
        job.remaining_time = (int64_t)burst_time;
        job.priority = (int)priority;
        job.completion_time = 0;
        job.turnaround_time = 0;
        job.waiting_time = 0;
        job.status = JOB_NEW;

        if (!job_list_add(list, &job)) {
            CC_ERROR("Line %d: failed to add job %d (validation failed or duplicate ID)", line_number, job.job_id);
            job_list_free(list);
            fclose(fp);
            return NULL;
        }
    }

    fclose(fp);
    return list;
}

void job_list_print(const JobList *list)
{
    if (!list) {
        printf("JobList is NULL\n");
        return;
    }

    print_section("Job List");
    printf("%-8s %-14s %-12s %-16s %-10s %-12s\n",
           "Job ID", "Arrival Time", "Burst Time", "Remaining Time", "Priority", "Status");
    print_separator();

    for (int i = 0; i < list->count; i++) {
        const Job *j = &list->jobs[i];
        const char *status_str = "UNKNOWN";
        switch (j->status) {
            case JOB_NEW:       status_str = "NEW"; break;
            case JOB_READY:     status_str = "READY"; break;
            case JOB_RUNNING:   status_str = "RUNNING"; break;
            case JOB_COMPLETED: status_str = "COMPLETED"; break;
        }
        printf("%-8d %-14" PRId64 " %-12" PRId64 " %-16" PRId64 " %-10d %-12s\n",
               j->job_id, j->arrival_time, j->burst_time, j->remaining_time,
               j->priority, status_str);
    }
    print_separator();
    printf("Total jobs: %d\n", list->count);
}
