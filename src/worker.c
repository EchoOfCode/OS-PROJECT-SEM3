/*
 * worker.c - Process management and worker execution demo for CampusCompute
 */
#define _POSIX_C_SOURCE 200809L

#include "worker.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

int worker_run_normal(void) {
    print_section("Worker Process: Normal Execution");

    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    if (pid < 0) {
        CC_ERROR("fork failed: %s", strerror(errno));
        return 1;
    }

    if (pid == 0) {
        /* Child process */
        pid_t my_pid = getpid();
        CC_INFO("[Child %d] Worker spawned. Starting simulated job computation...", (int)my_pid);

        /* Simulate lightweight computation / burst */
        volatile uint64_t sum = 0;
        for (uint64_t i = 1; i <= 100000; i++) {
            sum += i;
        }

        CC_INFO("[Child %d] Computation completed (checksum: %llu).",
                (int)my_pid, (unsigned long long)sum);
        CC_INFO("[Child %d] Exiting cleanly with status 0.", (int)my_pid);
        exit(0);
    }

    /* Parent process */
    CC_INFO("[Parent %d] Spawned child worker %d, waiting for exit status...",
            (int)getpid(), (int)pid);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        CC_ERROR("waitpid failed for child %d: %s", (int)pid, strerror(errno));
        return 1;
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        CC_INFO("[Parent] Worker %d exited normally with exit code %d.", (int)pid, exit_code);
        if (exit_code == 0) {
            CC_INFO("[Parent] Success: Worker completed successfully as expected.");
            return 0;
        } else {
            CC_ERROR("[Parent] Failure: Worker exited with unexpected status %d.", exit_code);
            return 1;
        }
    } else if (WIFSIGNALED(status)) {
        CC_WARN("[Parent] Worker %d was killed by signal %d (%s).",
                (int)pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
        return 1;
    } else {
        CC_ERROR("[Parent] Worker %d exited with unrecognized state.", (int)pid);
        return 1;
    }
}

int worker_run_fail(void) {
    print_section("Worker Process: Controlled Failure Handling");

    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    if (pid < 0) {
        CC_ERROR("fork failed: %s", strerror(errno));
        return 1;
    }

    if (pid == 0) {
        /* Child process */
        pid_t my_pid = getpid();
        CC_INFO("[Child %d] Worker spawned. Simulating task failure scenario...", (int)my_pid);
        CC_WARN("[Child %d] Simulated out-of-memory / unrecoverable exception encountered!", (int)my_pid);
        CC_INFO("[Child %d] Exiting with controlled non-zero failure code 42.", (int)my_pid);
        exit(42);
    }

    /* Parent process */
    CC_INFO("[Parent %d] Spawned child worker %d, waiting for exit status...",
            (int)getpid(), (int)pid);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        CC_ERROR("waitpid failed for child %d: %s", (int)pid, strerror(errno));
        return 1;
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        CC_INFO("[Parent] Worker %d exited with exit code %d.", (int)pid, exit_code);
        if (exit_code == 42) {
            CC_INFO("[Parent] Success: Controlled failure correctly detected and handled (exit status 42).");
            return 0;
        } else {
            CC_ERROR("[Parent] Failure: Worker exited with status %d (expected 42).", exit_code);
            return 1;
        }
    } else if (WIFSIGNALED(status)) {
        CC_WARN("[Parent] Worker %d was terminated by signal %d (%s).",
                (int)pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
        return 1;
    } else {
        CC_ERROR("[Parent] Worker %d exited with unrecognized state.", (int)pid);
        return 1;
    }
}
