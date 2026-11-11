/*
 * test_banker.c - Unit tests for Banker's Safety Algorithm
 */

#include "banker.h"
#include "common.h"
#include "../test_framework.h"

static void free_test_matrix(int **mat, int rows)
{
    if (!mat) return;
    for (int i = 0; i < rows; i++) {
        free(mat[i]);
    }
    free(mat);
}

/* Test 1: Verify safe sequence on known safe dataset */
TEST(test_banker_safe_sequence)
{
    int *avail = NULL;
    int **alloc = NULL;
    int **max_mat = NULL;
    int n_proc = 0;
    int n_res = 0;

    bool loaded = banker_load_file("data/banker_safe.txt", &avail, &alloc, &max_mat, &n_proc, &n_res);
    ASSERT_TRUE(loaded);
    ASSERT_EQ(n_proc, 5);
    ASSERT_EQ(n_res, 3);

    int *safe_seq = safe_malloc(sizeof(int) * (size_t)n_proc);
    bool is_safe = banker_check_safety(avail, alloc, max_mat, n_proc, n_res, safe_seq);
    ASSERT_TRUE(is_safe);

    /* In standard textbook 5-process 3-resource case:
       P1 is first (needs 1 2 2 <= 3 3 2)
       Verify that every process in safe_seq is within [0, 4] and all 5 are distinct */
    bool seen[5] = { false };
    for (int i = 0; i < n_proc; i++) {
        ASSERT_TRUE(safe_seq[i] >= 0 && safe_seq[i] < n_proc);
        ASSERT_FALSE(seen[safe_seq[i]]);
        seen[safe_seq[i]] = true;
    }

    free(safe_seq);
    free(avail);
    free_test_matrix(alloc, n_proc);
    free_test_matrix(max_mat, n_proc);
}

/* Test 2: Verify unsafe detection on unsafe dataset */
TEST(test_banker_unsafe_state)
{
    int *avail = NULL;
    int **alloc = NULL;
    int **max_mat = NULL;
    int n_proc = 0;
    int n_res = 0;

    bool loaded = banker_load_file("data/banker_unsafe.txt", &avail, &alloc, &max_mat, &n_proc, &n_res);
    ASSERT_TRUE(loaded);

    int *safe_seq = safe_malloc(sizeof(int) * (size_t)n_proc);
    bool is_safe = banker_check_safety(avail, alloc, max_mat, n_proc, n_res, safe_seq);
    ASSERT_FALSE(is_safe);

    free(safe_seq);
    free(avail);
    free_test_matrix(alloc, n_proc);
    free_test_matrix(max_mat, n_proc);
}

/* Test 3: Validation - allocation exceeding maximum */
TEST(test_banker_allocation_exceeds_max)
{
    int avail[2] = { 2, 2 };
    int *alloc[1];
    int alloc_row[2] = { 5, 1 }; /* 5 allocated, but max is 3 */
    alloc[0] = alloc_row;

    int *max_mat[1];
    int max_row[2] = { 3, 2 };
    max_mat[0] = max_row;

    int safe_seq[1] = { -1 };
    bool result = banker_check_safety(avail, alloc, max_mat, 1, 2, safe_seq);
    ASSERT_FALSE(result);
}

/* Test 4: File loading error handling */
TEST(test_banker_missing_file)
{
    int *avail = NULL;
    int **alloc = NULL;
    int **max_mat = NULL;
    int n_proc = 0;
    int n_res = 0;

    bool loaded = banker_load_file("data/nonexistent_banker.txt", &avail, &alloc, &max_mat, &n_proc, &n_res);
    ASSERT_FALSE(loaded);
}

/* Test 5: Full safe and unsafe runner execution */
TEST(test_banker_demo_runners)
{
    int ret_safe = banker_run_safe();
    ASSERT_EQ(ret_safe, 0);

    int ret_unsafe = banker_run_unsafe();
    ASSERT_EQ(ret_unsafe, 0);
}

int main(void)
{
    printf("Running unit tests: test_banker\n");
    printf("----------------------------------------\n");
    RUN_TEST(test_banker_safe_sequence);
    RUN_TEST(test_banker_unsafe_state);
    RUN_TEST(test_banker_allocation_exceeds_max);
    RUN_TEST(test_banker_missing_file);
    RUN_TEST(test_banker_demo_runners);

    TEST_SUMMARY("test_banker");
}
