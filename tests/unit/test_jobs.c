/*
 * test_jobs.c - Unit tests for job management module
 */

#include "jobs.h"
#include "common.h"
#include "../test_framework.h"

/* Test 1: Validate fields */
TEST(test_job_validate_fields)
{
    Job valid_job = {
        .job_id = 1,
        .arrival_time = 0,
        .burst_time = 5,
        .remaining_time = 5,
        .priority = 2,
        .status = JOB_NEW
    };
    ASSERT_TRUE(job_validate(&valid_job));

    /* Negative burst */
    Job invalid_burst = valid_job;
    invalid_burst.burst_time = -3;
    ASSERT_FALSE(job_validate(&invalid_burst));

    /* Zero burst */
    invalid_burst.burst_time = 0;
    ASSERT_FALSE(job_validate(&invalid_burst));

    /* Negative arrival */
    Job invalid_arrival = valid_job;
    invalid_arrival.arrival_time = -1;
    ASSERT_FALSE(job_validate(&invalid_arrival));

    /* Priority out of range (< 1 or > 10) */
    Job invalid_prio_low = valid_job;
    invalid_prio_low.priority = 0;
    ASSERT_FALSE(job_validate(&invalid_prio_low));

    Job invalid_prio_high = valid_job;
    invalid_prio_high.priority = 11;
    ASSERT_FALSE(job_validate(&invalid_prio_high));
}

/* Test 2: Reject duplicate IDs */
TEST(test_job_reject_duplicates)
{
    JobList *list = job_list_create(2);
    ASSERT_NOT_NULL(list);

    Job j1 = { .job_id = 10, .arrival_time = 0, .burst_time = 4, .priority = 2, .remaining_time = 4 };
    Job j2 = { .job_id = 10, .arrival_time = 2, .burst_time = 3, .priority = 5, .remaining_time = 3 };
    Job j3 = { .job_id = 20, .arrival_time = 2, .burst_time = 3, .priority = 5, .remaining_time = 3 };

    ASSERT_TRUE(job_list_add(list, &j1));
    ASSERT_EQ(list->count, 1);

    /* Second add with duplicate ID 10 should fail */
    ASSERT_FALSE(job_list_add(list, &j2));
    ASSERT_EQ(list->count, 1);

    /* Different ID should succeed */
    ASSERT_TRUE(job_list_add(list, &j3));
    ASSERT_EQ(list->count, 2);

    job_list_free(list);
}

/* Test 3: Reject negative burst during add */
TEST(test_job_reject_negative_burst)
{
    JobList *list = job_list_create(4);
    ASSERT_NOT_NULL(list);

    Job bad_job = { .job_id = 99, .arrival_time = 0, .burst_time = -5, .priority = 1 };
    ASSERT_FALSE(job_list_add(list, &bad_job));
    ASSERT_EQ(list->count, 0);

    job_list_free(list);
}

/* Test 4: Deep copy independence */
TEST(test_job_copy_independence)
{
    JobList *list = job_list_create(4);
    ASSERT_NOT_NULL(list);

    Job j1 = { .job_id = 1, .arrival_time = 0, .burst_time = 10, .priority = 3, .remaining_time = 10, .status = JOB_NEW };
    Job j2 = { .job_id = 2, .arrival_time = 1, .burst_time = 5, .priority = 2, .remaining_time = 5, .status = JOB_NEW };

    job_list_add(list, &j1);
    job_list_add(list, &j2);

    JobList *copied = job_list_copy(list);
    ASSERT_NOT_NULL(copied);
    ASSERT_EQ(copied->count, list->count);

    /* Mutating copied list must not affect original list */
    copied->jobs[0].remaining_time = 2;
    copied->jobs[0].status = JOB_RUNNING;

    ASSERT_EQ(list->jobs[0].remaining_time, 10);
    ASSERT_EQ(list->jobs[0].status, JOB_NEW);
    ASSERT_EQ(copied->jobs[0].remaining_time, 2);
    ASSERT_EQ(copied->jobs[0].status, JOB_RUNNING);

    job_list_free(list);
    job_list_free(copied);
}

/* Test 5: Load CSV file */
TEST(test_job_load_csv)
{
    JobList *list = job_list_load_csv("data/jobs.csv");
    ASSERT_NOT_NULL(list);
    ASSERT_EQ(list->count, 4);

    /* Check job 1: 1,0,5,2 */
    ASSERT_EQ(list->jobs[0].job_id, 1);
    ASSERT_EQ(list->jobs[0].arrival_time, 0);
    ASSERT_EQ(list->jobs[0].burst_time, 5);
    ASSERT_EQ(list->jobs[0].remaining_time, 5);
    ASSERT_EQ(list->jobs[0].priority, 2);

    /* Check job 4: 4,3,6,2 */
    ASSERT_EQ(list->jobs[3].job_id, 4);
    ASSERT_EQ(list->jobs[3].arrival_time, 3);
    ASSERT_EQ(list->jobs[3].burst_time, 6);
    ASSERT_EQ(list->jobs[3].priority, 2);

    job_list_free(list);

    /* Loading non-existent file returns NULL */
    JobList *missing = job_list_load_csv("data/nonexistent_file.csv");
    ASSERT_NULL(missing);
}

int main(void)
{
    printf("Running unit tests: test_jobs\n");
    printf("----------------------------------------\n");
    RUN_TEST(test_job_validate_fields);
    RUN_TEST(test_job_reject_duplicates);
    RUN_TEST(test_job_reject_negative_burst);
    RUN_TEST(test_job_copy_independence);
    RUN_TEST(test_job_load_csv);

    TEST_SUMMARY("test_jobs");
}
