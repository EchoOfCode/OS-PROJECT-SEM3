/*
 * test_scheduler.c - Unit tests for CPU scheduling algorithms
 */

#include "scheduler.h"
#include "jobs.h"
#include "common.h"
#include "../test_framework.h"

/* Helper to find job index in ScheduleResult */
static int find_job_idx(const ScheduleResult *r, int job_id)
{
    for (int i = 0; i < r->job_count; i++) {
        if (r->job_ids[i] == job_id) {
            return i;
        }
    }
    return -1;
}

/* Test 1: FCFS scheduling calculations for jobs.csv */
TEST(test_scheduler_fcfs_jobs_csv)
{
    JobList *list = job_list_load_csv("data/jobs.csv");
    ASSERT_NOT_NULL(list);

    ScheduleResult res = schedule_fcfs(list);
    ASSERT_EQ(res.job_count, 4);

    int idx1 = find_job_idx(&res, 1);
    int idx2 = find_job_idx(&res, 2);
    int idx3 = find_job_idx(&res, 3);
    int idx4 = find_job_idx(&res, 4);

    ASSERT_TRUE(idx1 >= 0 && idx2 >= 0 && idx3 >= 0 && idx4 >= 0);

    /* CT: J1=5, J2=8, J3=16, J4=22 */
    ASSERT_EQ(res.completion_times[idx1], 5);
    ASSERT_EQ(res.completion_times[idx2], 8);
    ASSERT_EQ(res.completion_times[idx3], 16);
    ASSERT_EQ(res.completion_times[idx4], 22);

    /* TAT: J1=5, J2=7, J3=14, J4=19 */
    ASSERT_EQ(res.turnaround_times[idx1], 5);
    ASSERT_EQ(res.turnaround_times[idx2], 7);
    ASSERT_EQ(res.turnaround_times[idx3], 14);
    ASSERT_EQ(res.turnaround_times[idx4], 19);

    /* WT: J1=0, J2=4, J3=6, J4=13 */
    ASSERT_EQ(res.waiting_times[idx1], 0);
    ASSERT_EQ(res.waiting_times[idx2], 4);
    ASSERT_EQ(res.waiting_times[idx3], 6);
    ASSERT_EQ(res.waiting_times[idx4], 13);

    /* Averages */
    ASSERT_DOUBLE_NEAR(res.avg_turnaround, 11.25, 0.001);
    ASSERT_DOUBLE_NEAR(res.avg_waiting, 5.75, 0.001);

    schedule_result_free(&res);
    job_list_free(list);
}

/* Test 2: Priority scheduling calculations for jobs.csv */
TEST(test_scheduler_priority_jobs_csv)
{
    JobList *list = job_list_load_csv("data/jobs.csv");
    ASSERT_NOT_NULL(list);

    ScheduleResult res = schedule_priority(list);
    ASSERT_EQ(res.job_count, 4);

    int idx1 = find_job_idx(&res, 1);
    int idx2 = find_job_idx(&res, 2);
    int idx3 = find_job_idx(&res, 3);
    int idx4 = find_job_idx(&res, 4);

    ASSERT_TRUE(idx1 >= 0 && idx2 >= 0 && idx3 >= 0 && idx4 >= 0);

    /* Execution order: J1 (0-5), J2 (5-8, prio 1), J4 (8-14, prio 2), J3 (14-22, prio 3) */
    ASSERT_EQ(res.completion_times[idx1], 5);
    ASSERT_EQ(res.completion_times[idx2], 8);
    ASSERT_EQ(res.completion_times[idx4], 14);
    ASSERT_EQ(res.completion_times[idx3], 22);

    ASSERT_EQ(res.turnaround_times[idx1], 5);
    ASSERT_EQ(res.turnaround_times[idx2], 7);
    ASSERT_EQ(res.turnaround_times[idx4], 11);
    ASSERT_EQ(res.turnaround_times[idx3], 20);

    ASSERT_EQ(res.waiting_times[idx1], 0);
    ASSERT_EQ(res.waiting_times[idx2], 4);
    ASSERT_EQ(res.waiting_times[idx4], 5);
    ASSERT_EQ(res.waiting_times[idx3], 12);

    ASSERT_DOUBLE_NEAR(res.avg_turnaround, 10.75, 0.001);
    ASSERT_DOUBLE_NEAR(res.avg_waiting, 5.25, 0.001);

    schedule_result_free(&res);
    job_list_free(list);
}

/* Test 3: Round Robin scheduling calculations for jobs.csv (quantum = 2) */
TEST(test_scheduler_rr_jobs_csv)
{
    JobList *list = job_list_load_csv("data/jobs.csv");
    ASSERT_NOT_NULL(list);

    ScheduleResult res = schedule_rr(list, 2);
    ASSERT_EQ(res.job_count, 4);

    int idx1 = find_job_idx(&res, 1);
    int idx2 = find_job_idx(&res, 2);
    int idx3 = find_job_idx(&res, 3);
    int idx4 = find_job_idx(&res, 4);

    ASSERT_TRUE(idx1 >= 0 && idx2 >= 0 && idx3 >= 0 && idx4 >= 0);

    /* CT: J1=14, J2=11, J3=22, J4=20 */
    ASSERT_EQ(res.completion_times[idx1], 14);
    ASSERT_EQ(res.completion_times[idx2], 11);
    ASSERT_EQ(res.completion_times[idx3], 22);
    ASSERT_EQ(res.completion_times[idx4], 20);

    /* TAT: J1=14, J2=10, J3=20, J4=17 */
    ASSERT_EQ(res.turnaround_times[idx1], 14);
    ASSERT_EQ(res.turnaround_times[idx2], 10);
    ASSERT_EQ(res.turnaround_times[idx3], 20);
    ASSERT_EQ(res.turnaround_times[idx4], 17);

    /* WT: J1=9, J2=7, J3=12, J4=11 */
    ASSERT_EQ(res.waiting_times[idx1], 9);
    ASSERT_EQ(res.waiting_times[idx2], 7);
    ASSERT_EQ(res.waiting_times[idx3], 12);
    ASSERT_EQ(res.waiting_times[idx4], 11);

    ASSERT_DOUBLE_NEAR(res.avg_turnaround, 15.25, 0.001);
    ASSERT_DOUBLE_NEAR(res.avg_waiting, 9.75, 0.001);

    schedule_result_free(&res);
    job_list_free(list);
}

/* Test 4: Single job scheduling */
TEST(test_scheduler_single_job)
{
    JobList *list = job_list_create(1);
    Job j = { .job_id = 42, .arrival_time = 5, .burst_time = 7, .remaining_time = 7, .priority = 1, .status = JOB_NEW };
    job_list_add(list, &j);

    ScheduleResult res = schedule_fcfs(list);
    ASSERT_EQ(res.job_count, 1);
    ASSERT_EQ(res.completion_times[0], 12);
    ASSERT_EQ(res.turnaround_times[0], 7);
    ASSERT_EQ(res.waiting_times[0], 0);

    schedule_result_free(&res);
    job_list_free(list);
}

/* Test 5: Empty input */
TEST(test_scheduler_empty_input)
{
    JobList *list = job_list_create(2);
    ScheduleResult res = schedule_fcfs(list);
    ASSERT_EQ(res.job_count, 0);
    ASSERT_DOUBLE_NEAR(res.avg_turnaround, 0.0, 0.001);
    schedule_result_free(&res);

    ScheduleResult res_rr = schedule_rr(list, 2);
    ASSERT_EQ(res_rr.job_count, 0);
    schedule_result_free(&res_rr);

    ScheduleResult res_prio = schedule_priority(list);
    ASSERT_EQ(res_prio.job_count, 0);
    schedule_result_free(&res_prio);

    job_list_free(list);
}

/* Test 6: Equal arrival times */
TEST(test_scheduler_equal_arrival_times)
{
    JobList *list = job_list_create(2);
    Job j1 = { .job_id = 1, .arrival_time = 0, .burst_time = 4, .remaining_time = 4, .priority = 2, .status = JOB_NEW };
    Job j2 = { .job_id = 2, .arrival_time = 0, .burst_time = 2, .remaining_time = 2, .priority = 1, .status = JOB_NEW };
    job_list_add(list, &j1);
    job_list_add(list, &j2);

    /* Under Priority scheduling, J2 (prio 1) must run first */
    ScheduleResult res_prio = schedule_priority(list);
    int idx1 = find_job_idx(&res_prio, 1);
    int idx2 = find_job_idx(&res_prio, 2);
    ASSERT_EQ(res_prio.completion_times[idx2], 2);
    ASSERT_EQ(res_prio.completion_times[idx1], 6);

    schedule_result_free(&res_prio);
    job_list_free(list);
}

/* Test 7: Idle CPU periods (gap before next job arrives) */
TEST(test_scheduler_idle_periods)
{
    JobList *list = job_list_create(2);
    Job j1 = { .job_id = 1, .arrival_time = 0, .burst_time = 3, .remaining_time = 3, .priority = 1, .status = JOB_NEW };
    Job j2 = { .job_id = 2, .arrival_time = 10, .burst_time = 4, .remaining_time = 4, .priority = 1, .status = JOB_NEW };
    job_list_add(list, &j1);
    job_list_add(list, &j2);

    ScheduleResult res = schedule_fcfs(list);
    int idx1 = find_job_idx(&res, 1);
    int idx2 = find_job_idx(&res, 2);

    ASSERT_EQ(res.completion_times[idx1], 3);
    /* J2 arrives at 10, runs from 10 to 14 */
    ASSERT_EQ(res.completion_times[idx2], 14);
    ASSERT_EQ(res.waiting_times[idx2], 0);
    ASSERT_EQ(res.turnaround_times[idx2], 4);

    schedule_result_free(&res);
    job_list_free(list);
}

/* Test 8: Quantum validation for Round Robin */
TEST(test_scheduler_quantum_validation)
{
    JobList *list = job_list_load_csv("data/jobs.csv");
    ASSERT_NOT_NULL(list);

    /* Quantum <= 0 should return empty result */
    ScheduleResult bad_res = schedule_rr(list, 0);
    ASSERT_EQ(bad_res.job_count, 0);
    schedule_result_free(&bad_res);

    ScheduleResult neg_res = schedule_rr(list, -2);
    ASSERT_EQ(neg_res.job_count, 0);
    schedule_result_free(&neg_res);

    job_list_free(list);
}

int main(void)
{
    printf("Running unit tests: test_scheduler\n");
    printf("----------------------------------------\n");
    RUN_TEST(test_scheduler_fcfs_jobs_csv);
    RUN_TEST(test_scheduler_priority_jobs_csv);
    RUN_TEST(test_scheduler_rr_jobs_csv);
    RUN_TEST(test_scheduler_single_job);
    RUN_TEST(test_scheduler_empty_input);
    RUN_TEST(test_scheduler_equal_arrival_times);
    RUN_TEST(test_scheduler_idle_periods);
    RUN_TEST(test_scheduler_quantum_validation);

    TEST_SUMMARY("test_scheduler");
}
