/*
 * logger.h - Concurrent producer-consumer activity logger for CampusCompute
 */
#ifndef LOGGER_H
#define LOGGER_H

#include "common.h"
#include <pthread.h>

#define LOG_MSG_MAX 256

/* A single log event */
typedef struct {
    char message[LOG_MSG_MAX];
    int producer_id;
    int event_id;
} LogEvent;

/* Bounded thread-safe queue */
typedef struct {
    LogEvent *buffer;
    int capacity;
    int count;
    int head;
    int tail;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    bool producers_done;
    int active_producers;
} LogQueue;

/* Configuration for logger run */
typedef struct {
    int num_producers;
    int events_per_producer;
    int queue_capacity;
    const char *output_path;
} LoggerConfig;

/* Queue management */
bool log_queue_init(LogQueue *q, int capacity);
void log_queue_destroy(LogQueue *q);
void log_queue_enqueue(LogQueue *q, LogEvent *event);
bool log_queue_dequeue(LogQueue *q, LogEvent *event);

/* Thread functions */
void *producer_thread(void *arg);
void *consumer_thread(void *arg);

/* Runner */
int logger_run(LoggerConfig *config);

#endif /* LOGGER_H */
