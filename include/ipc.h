/*
 * ipc.h - Shared-memory IPC for CampusCompute
 */
#ifndef IPC_H
#define IPC_H

#include "common.h"
#include <sys/types.h>
#include <semaphore.h>

#define IPC_SHM_NAME_MAX 64
#define IPC_PAYLOAD_MSG_MAX 256

/* Payload exchanged via shared memory between parent and workers */
typedef struct {
    int job_id;
    char request[IPC_PAYLOAD_MSG_MAX];
    char response[IPC_PAYLOAD_MSG_MAX];
    int status;
    bool completed;
} SharedPayload;

/* IPC demo */
int ipc_demo(void);

#endif /* IPC_H */
