/*
 * resource_limits.c - setrlimit demo for CampusCompute
 */
#define _POSIX_C_SOURCE 200809L

#include "resource_limits.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

int resource_limits_demo(void) {
    print_section("Resource Limits Demonstration (setrlimit / RLIMIT_NOFILE)");

    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    if (pid < 0) {
        CC_ERROR("fork failed: %s", strerror(errno));
        return 1;
    }

    if (pid == 0) {
        /* Child process executes setrlimit demonstration */
        pid_t my_pid = getpid();
        struct rlimit rl;

        if (getrlimit(RLIMIT_NOFILE, &rl) != 0) {
            CC_ERROR("[Child %d] getrlimit(RLIMIT_NOFILE) failed: %s",
                     (int)my_pid, strerror(errno));
            exit(1);
        }

        CC_INFO("[Child %d] Original RLIMIT_NOFILE: soft=%llu, hard=%llu",
                (int)my_pid,
                (unsigned long long)rl.rlim_cur,
                (unsigned long long)rl.rlim_max);

        /* Set new soft limit = min(20, current_hard) */
        rlim_t target_soft = 20;
        if (target_soft > rl.rlim_max) {
            target_soft = rl.rlim_max;
        }

        rl.rlim_cur = target_soft;
        if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
            CC_ERROR("[Child %d] setrlimit(RLIMIT_NOFILE) failed: %s",
                     (int)my_pid, strerror(errno));
            exit(1);
        }

        /* Verify new limits */
        struct rlimit new_rl;
        if (getrlimit(RLIMIT_NOFILE, &new_rl) != 0) {
            CC_ERROR("[Child %d] getrlimit verification failed: %s",
                     (int)my_pid, strerror(errno));
            exit(1);
        }

        CC_INFO("[Child %d] Configured new RLIMIT_NOFILE: soft=%llu, hard=%llu",
                (int)my_pid,
                (unsigned long long)new_rl.rlim_cur,
                (unsigned long long)new_rl.rlim_max);

        /* Open /dev/null in a loop until EMFILE is encountered */
        int fds[128];
        int opened_count = 0;
        int hit_emfile = 0;

        while (opened_count < 128) {
            int fd = open("/dev/null", O_RDONLY);
            if (fd < 0) {
                if (errno == EMFILE) {
                    CC_INFO("[Child %d] Reached open file limit (EMFILE): %s",
                            (int)my_pid, strerror(errno));
                    hit_emfile = 1;
                } else {
                    CC_ERROR("[Child %d] open failed with unexpected error: %s",
                             (int)my_pid, strerror(errno));
                }
                break;
            }
            fds[opened_count++] = fd;
        }

        CC_INFO("[Child %d] Successfully opened %d file descriptors before exhaustion.",
                (int)my_pid, opened_count);

        /* Close all opened file descriptors */
        for (int i = 0; i < opened_count; i++) {
            close(fds[i]);
        }
        CC_INFO("[Child %d] Cleaned up: closed all %d open file descriptors.",
                (int)my_pid, opened_count);

        if (!hit_emfile) {
            CC_WARN("[Child %d] Loop terminated without reaching EMFILE limit.", (int)my_pid);
            exit(1);
        }

        exit(0);
    }

    /* Parent process */
    CC_INFO("[Parent %d] Spawned child %d for isolated setrlimit testing, waiting...",
            (int)getpid(), (int)pid);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        CC_ERROR("waitpid failed: %s", strerror(errno));
        return 1;
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code == 0) {
            CC_INFO("[Parent] Resource limits demo completed successfully (child exit code 0).");
            return 0;
        } else {
            CC_ERROR("[Parent] Resource limits demo failed (child exit code %d).", exit_code);
            return 1;
        }
    } else {
        CC_ERROR("[Parent] Child terminated abnormally.");
        return 1;
    }
}
