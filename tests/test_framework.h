/*
 * test_framework.h - Minimalist unit testing framework for CampusCompute
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST(name) static void name(void)

#define RUN_TEST(func) do { \
    g_tests_run++; \
    int failed_before = g_tests_failed; \
    printf("  [RUN ] %s\n", #func); \
    func(); \
    if (g_tests_failed == failed_before) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", #func); \
    } else { \
        printf("  [FAIL] %s\n", #func); \
    } \
} while (0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "    Assertion Failed at %s:%d: '%s' is not true\n", __FILE__, __LINE__, #cond); \
        g_tests_failed++; \
        return; \
    } \
} while (0)

#define ASSERT_FALSE(cond) do { \
    if (cond) { \
        fprintf(stderr, "    Assertion Failed at %s:%d: '%s' is not false\n", __FILE__, __LINE__, #cond); \
        g_tests_failed++; \
        return; \
    } \
} while (0)

#define ASSERT_EQ(a, b) do { \
    long long _va = (long long)(a); \
    long long _vb = (long long)(b); \
    if (_va != _vb) { \
        fprintf(stderr, "    Assertion Failed at %s:%d: %s (%lld) != %s (%lld)\n", __FILE__, __LINE__, #a, _va, #b, _vb); \
        g_tests_failed++; \
        return; \
    } \
} while (0)

#define ASSERT_DOUBLE_NEAR(a, b, eps) do { \
    double _da = (double)(a); \
    double _db = (double)(b); \
    double _diff = (_da > _db) ? (_da - _db) : (_db - _da); \
    if (_diff > (eps)) { \
        fprintf(stderr, "    Assertion Failed at %s:%d: |%f - %f| = %f > %f\n", __FILE__, __LINE__, _da, _db, _diff, (double)(eps)); \
        g_tests_failed++; \
        return; \
    } \
} while (0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, "    Assertion Failed at %s:%d: '%s' is NULL\n", __FILE__, __LINE__, #ptr); \
        g_tests_failed++; \
        return; \
    } \
} while (0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        fprintf(stderr, "    Assertion Failed at %s:%d: '%s' is not NULL\n", __FILE__, __LINE__, #ptr); \
        g_tests_failed++; \
        return; \
    } \
} while (0)

#define TEST_SUMMARY(suite_name) do { \
    printf("\n=== %s Summary: %d Run, %d Passed, %d Failed ===\n\n", \
           suite_name, g_tests_run, g_tests_passed, g_tests_failed); \
    if (g_tests_failed > 0) { \
        return 1; \
    } \
    return 0; \
} while (0)

#endif /* TEST_FRAMEWORK_H */
