/*
 * ipc.c - POSIX Shared Memory IPC demo for CampusCompute
 */
#define _POSIX_C_SOURCE 200809L

#include "ipc.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>

int ipc_demo(void) {
    print_section("POSIX Shared Memory & Named Semaphores IPC Demo");

    pid_t parent_pid = getpid();
    char shm_name[IPC_SHM_NAME_MAX];
    char sem_req_name[IPC_SHM_NAME_MAX];
    char sem_resp_name[IPC_SHM_NAME_MAX];

    snprintf(shm_name, sizeof(shm_name), "/cc_shm_%d", (int)parent_pid);
    snprintf(sem_req_name, sizeof(sem_req_name), "/cc_sem_req_%d", (int)parent_pid);
    snprintf(sem_resp_name, sizeof(sem_resp_name), "/cc_sem_resp_%d", (int)parent_pid);

    /* Unlink any existing stale objects with identical names */
    shm_unlink(shm_name);
    sem_unlink(sem_req_name);
    sem_unlink(sem_resp_name);

    CC_INFO("[Parent %d] Creating shared memory '%s'...", (int)parent_pid, shm_name);
    int shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd < 0) {
        CC_ERROR("shm_open failed: %s", strerror(errno));
        return 1;
    }

    if (ftruncate(shm_fd, (off_t)sizeof(SharedPayload)) == -1) {
        CC_ERROR("ftruncate failed: %s", strerror(errno));
        close(shm_fd);
        shm_unlink(shm_name);
        return 1;
    }

    SharedPayload *payload = (SharedPayload *)mmap(
        NULL, sizeof(SharedPayload), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (payload == MAP_FAILED) {
        CC_ERROR("mmap failed: %s", strerror(errno));
        close(shm_fd);
        shm_unlink(shm_name);
        return 1;
    }

    memset(payload, 0, sizeof(SharedPayload));

    CC_INFO("[Parent %d] Creating named semaphores '%s' and '%s'...",
            (int)parent_pid, sem_req_name, sem_resp_name);
    sem_t *sem_req = sem_open(sem_req_name, O_CREAT | O_EXCL, 0666, 0);
    if (sem_req == SEM_FAILED) {
        CC_ERROR("sem_open(req) failed: %s", strerror(errno));
        munmap(payload, sizeof(SharedPayload));
        close(shm_fd);
        shm_unlink(shm_name);
        return 1;
    }

    sem_t *sem_resp = sem_open(sem_resp_name, O_CREAT | O_EXCL, 0666, 0);
    if (sem_resp == SEM_FAILED) {
        CC_ERROR("sem_open(resp) failed: %s", strerror(errno));
        sem_close(sem_req);
        sem_unlink(sem_req_name);
        munmap(payload, sizeof(SharedPayload));
        close(shm_fd);
        shm_unlink(shm_name);
        return 1;
    }

    /* Parent populates request data */
    payload->job_id = 42;
    snprintf(payload->request, sizeof(payload->request), "Process job 42");
    payload->status = 0;
    payload->completed = false;

    CC_INFO("[Parent %d] Prepared request: job_id=%d, request=\"%s\"",
            (int)parent_pid, payload->job_id, payload->request);

    fflush(stdout);
    fflush(stderr);

    pid_t child_pid = fork();
    if (child_pid < 0) {
        CC_ERROR("fork failed: %s", strerror(errno));
        sem_close(sem_req);
        sem_close(sem_resp);
        sem_unlink(sem_req_name);
        sem_unlink(sem_resp_name);
        munmap(payload, sizeof(SharedPayload));
        close(shm_fd);
        shm_unlink(shm_name);
        return 1;
    }

    if (child_pid == 0) {
        /* Child process: unmap parent's handles and open by name to demonstrate full IPC */
        munmap(payload, sizeof(SharedPayload));
        close(shm_fd);
        sem_close(sem_req);
        sem_close(sem_resp);

        pid_t c_pid = getpid();
        CC_INFO("[Child %d] Opening shared memory '%s'...", (int)c_pid, shm_name);
        int c_shm_fd = shm_open(shm_name, O_RDWR, 0666);
        if (c_shm_fd < 0) {
            CC_ERROR("[Child %d] shm_open failed: %s", (int)c_pid, strerror(errno));
            _exit(1);
        }

        SharedPayload *c_payload = (SharedPayload *)mmap(
            NULL, sizeof(SharedPayload), PROT_READ | PROT_WRITE, MAP_SHARED, c_shm_fd, 0);
        if (c_payload == MAP_FAILED) {
            CC_ERROR("[Child %d] mmap failed: %s", (int)c_pid, strerror(errno));
            close(c_shm_fd);
            _exit(1);
        }

        sem_t *c_sem_req = sem_open(sem_req_name, 0);
        sem_t *c_sem_resp = sem_open(sem_resp_name, 0);
        if (c_sem_req == SEM_FAILED || c_sem_resp == SEM_FAILED) {
            CC_ERROR("[Child %d] sem_open failed: %s", (int)c_pid, strerror(errno));
            munmap(c_payload, sizeof(SharedPayload));
            close(c_shm_fd);
            _exit(1);
        }

        CC_INFO("[Child %d] Waiting on sem_req for parent's job request...", (int)c_pid);
        while (sem_wait(c_sem_req) < 0) {
            if (errno == EINTR) {
                continue;
            }
            CC_ERROR("[Child %d] sem_wait failed: %s", (int)c_pid, strerror(errno));
            _exit(1);
        }

        CC_INFO("[Child %d] Received request: job_id=%d, request=\"%s\"",
                (int)c_pid, c_payload->job_id, c_payload->request);

        /* Process request */
        snprintf(c_payload->response, sizeof(c_payload->response), "Job 42 completed successfully");
        c_payload->status = 1;
        c_payload->completed = true;

        CC_INFO("[Child %d] Work completed. Posted response: status=%d, response=\"%s\"",
                (int)c_pid, c_payload->status, c_payload->response);

        if (sem_post(c_sem_resp) < 0) {
            CC_ERROR("[Child %d] sem_post failed: %s", (int)c_pid, strerror(errno));
        }

        /* Child cleans up its descriptors */
        munmap(c_payload, sizeof(SharedPayload));
        close(c_shm_fd);
        sem_close(c_sem_req);
        sem_close(c_sem_resp);
        _exit(0);
    }

    /* Parent process */
    CC_INFO("[Parent %d] Signalling child by posting sem_req...", (int)parent_pid);
    if (sem_post(sem_req) < 0) {
        CC_ERROR("sem_post failed: %s", strerror(errno));
    }

    CC_INFO("[Parent %d] Waiting on sem_resp for child's response...", (int)parent_pid);
    while (sem_wait(sem_resp) < 0) {
        if (errno == EINTR) {
            continue;
        }
        CC_ERROR("sem_wait failed: %s", strerror(errno));
        break;
    }

    CC_INFO("[Parent %d] Received response: status=%d, response=\"%s\"",
            (int)parent_pid, payload->status, payload->response);

    /* Reap child process */
    int child_status = 0;
    while (waitpid(child_pid, &child_status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        CC_ERROR("waitpid failed: %s", strerror(errno));
        break;
    }

    if (WIFEXITED(child_status)) {
        CC_INFO("[Parent %d] Child process %d reaped successfully (exit status: %d).",
                (int)parent_pid, (int)child_pid, WEXITSTATUS(child_status));
    }

    /* Parent cleanup */
    munmap(payload, sizeof(SharedPayload));
    close(shm_fd);
    sem_close(sem_req);
    sem_close(sem_resp);

    if (sem_unlink(sem_req_name) != 0) {
        CC_WARN("sem_unlink(%s) failed: %s", sem_req_name, strerror(errno));
    }
    if (sem_unlink(sem_resp_name) != 0) {
        CC_WARN("sem_unlink(%s) failed: %s", sem_resp_name, strerror(errno));
    }
    if (shm_unlink(shm_name) != 0) {
        CC_WARN("shm_unlink(%s) failed: %s", shm_name, strerror(errno));
    }

    CC_INFO("[Parent %d] IPC demo completed successfully. All shared resources released.",
            (int)parent_pid);
    return 0;
}
