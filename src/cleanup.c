/*
 * cleanup.c - System resource cleanup demonstration for CampusCompute
 */
#define _POSIX_C_SOURCE 200809L

#include "cleanup.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

typedef struct {
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    int signaled;
} ThreadData;

static void *cleanup_thread_routine(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    pthread_mutex_lock(data->mutex);
    data->signaled = 1;
    pthread_cond_signal(data->cond);
    pthread_mutex_unlock(data->mutex);
    return NULL;
}

int cleanup_demo(void) {
    print_section("Resource Creation & Systematic Cleanup Demonstration");

    pid_t pid = getpid();
    int all_passed = 1;

    /* 1 & 2. POSIX Mutex and Condition Variable */
    CC_INFO("[1/7] Initializing POSIX Mutex and Condition Variable...");
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int rc_mutex_init = pthread_mutex_init(&mutex, NULL);
    int rc_cond_init = pthread_cond_init(&cond, NULL);

    /* 3. POSIX Thread */
    CC_INFO("[2/7] Creating and synchronizing POSIX Thread...");
    pthread_t thread;
    ThreadData tdata = { .mutex = &mutex, .cond = &cond, .signaled = 0 };

    pthread_mutex_lock(&mutex);
    int rc_thread_create = pthread_create(&thread, NULL, cleanup_thread_routine, &tdata);
    if (rc_thread_create == 0) {
        while (!tdata.signaled) {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);

    int rc_thread_join = -1;
    if (rc_thread_create == 0) {
        rc_thread_join = pthread_join(thread, NULL);
    }

    /* Destroy Mutex and Condvar */
    int rc_mutex_destroy = pthread_mutex_destroy(&mutex);
    int rc_cond_destroy = pthread_cond_destroy(&cond);

    /* 4. Child Process Creation & Reap */
    CC_INFO("[3/7] Forking and reaping Child Process...");
    fflush(stdout);
    pid_t child_pid = fork();
    int child_reaped = 0;
    int child_status = 0;

    if (child_pid == 0) {
        /* Child exits immediately */
        _exit(0);
    } else if (child_pid > 0) {
        while (waitpid(child_pid, &child_status, 0) < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0) {
            child_reaped = 1;
        }
    }

    /* 5. Temporary File */
    CC_INFO("[4/7] Creating and unlinking Temporary File...");
    char temp_path[128];
    snprintf(temp_path, sizeof(temp_path), "/tmp/cc_cleanup_%d.tmp", (int)pid);

    int file_cleanup_verified = 0;
    FILE *fp = fopen(temp_path, "w");
    if (fp != NULL) {
        fputs("CampusCompute cleanup demonstration temporary payload\n", fp);
        fclose(fp);

        if (unlink(temp_path) == 0) {
            /* Verify file cannot be opened */
            FILE *check_fp = fopen(temp_path, "r");
            if (check_fp == NULL && errno == ENOENT) {
                file_cleanup_verified = 1;
            } else if (check_fp != NULL) {
                fclose(check_fp);
            }
        }
    }

    /* 6. Shared Memory Segment */
    CC_INFO("[5/7] Creating, unlinking, and verifying Shared Memory...");
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "/cc_clean_shm_%d", (int)pid);
    shm_unlink(shm_name);

    int shm_cleanup_verified = 0;
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd >= 0) {
        if (ftruncate(shm_fd, 128) == 0) {
            close(shm_fd);
            if (shm_unlink(shm_name) == 0) {
                /* Verify subsequent open fails with ENOENT */
                int test_fd = shm_open(shm_name, O_RDWR, 0666);
                if (test_fd < 0 && errno == ENOENT) {
                    shm_cleanup_verified = 1;
                } else if (test_fd >= 0) {
                    close(test_fd);
                    shm_unlink(shm_name);
                }
            }
        } else {
            close(shm_fd);
            shm_unlink(shm_name);
        }
    }

    /* 7. Named Semaphore */
    CC_INFO("[6/7] Creating, unlinking, and verifying POSIX Semaphore...");
    char sem_name[64];
    snprintf(sem_name, sizeof(sem_name), "/cc_clean_sem_%d", (int)pid);
    sem_unlink(sem_name);

    int sem_cleanup_verified = 0;
    sem_t *sem = sem_open(sem_name, O_CREAT, 0666, 1);
    if (sem != SEM_FAILED) {
        sem_close(sem);
        if (sem_unlink(sem_name) == 0) {
            /* Verify opening unlinked semaphore fails */
            sem_t *test_sem = sem_open(sem_name, 0);
            if (test_sem == SEM_FAILED && errno == ENOENT) {
                sem_cleanup_verified = 1;
            } else if (test_sem != SEM_FAILED) {
                sem_close(test_sem);
                sem_unlink(sem_name);
            }
        }
    }

    /* Verification Summary */
    CC_INFO("\n[7/7] Generating Comprehensive Cleanup Verification Summary...");
    print_separator();
    printf("%-20s %-32s %-16s\n", "Resource Type", "Cleanup Mechanism", "Status");
    print_separator();

    printf("%-20s %-32s %-16s\n", "POSIX Mutex", "pthread_mutex_destroy()",
           (rc_mutex_init == 0 && rc_mutex_destroy == 0) ? "CLEANED (0)" : "FAILED");
    if (rc_mutex_destroy != 0) all_passed = 0;

    printf("%-20s %-32s %-16s\n", "POSIX Condvar", "pthread_cond_destroy()",
           (rc_cond_init == 0 && rc_cond_destroy == 0) ? "CLEANED (0)" : "FAILED");
    if (rc_cond_destroy != 0) all_passed = 0;

    printf("%-20s %-32s %-16s\n", "POSIX Thread", "pthread_join()",
           (rc_thread_create == 0 && rc_thread_join == 0) ? "CLEANED (0)" : "FAILED");
    if (rc_thread_join != 0) all_passed = 0;

    printf("%-20s %-32s %-16s\n", "Child Process", "waitpid() [reaped]",
           child_reaped ? "CLEANED (REAPED)" : "FAILED");
    if (!child_reaped) all_passed = 0;

    printf("%-20s %-32s %-16s\n", "Temp File", "unlink() [verified ENOENT]",
           file_cleanup_verified ? "CLEANED" : "FAILED");
    if (!file_cleanup_verified) all_passed = 0;

    printf("%-20s %-32s %-16s\n", "Shared Memory", "shm_unlink() [verified ENOENT]",
           shm_cleanup_verified ? "CLEANED" : "FAILED");
    if (!shm_cleanup_verified) all_passed = 0;

    printf("%-20s %-32s %-16s\n", "POSIX Semaphore", "sem_unlink() [verified ENOENT]",
           sem_cleanup_verified ? "CLEANED" : "FAILED");
    if (!sem_cleanup_verified) all_passed = 0;

    print_separator();

    if (all_passed) {
        CC_INFO("All resources successfully allocated, exercised, and destroyed.");
        CC_INFO("Zero resource leaks verified.");
        return 0;
    } else {
        CC_ERROR("One or more resources failed cleanup verification.");
        return 1;
    }
}
