/*
 * logger.c - Concurrent producer-consumer activity logger for CampusCompute
 *
 * Implements a bounded, thread-safe circular queue using POSIX threads,
 * a mutex, and condition variables (not_full and not_empty). Demonstrates
 * coordinated multi-producer single-consumer concurrency without busy waiting.
 */

#define _POSIX_C_SOURCE 200809L

#include "logger.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
#endif

/* ------------------------------------------------------------------ */
/*  Internal Thread Argument Structures                               */
/* ------------------------------------------------------------------ */

typedef struct {
    LogQueue *queue;
    int producer_id;
    int events_per_producer;
} ProducerArg;

typedef struct {
    LogQueue *queue;
    FILE *output_file;
    int events_consumed;
} ConsumerArg;

/* ------------------------------------------------------------------ */
/*  Helper Functions                                                  */
/* ------------------------------------------------------------------ */

static void brief_pause_ms(long ms)
{
#if defined(_WIN32)
    Sleep((DWORD)ms);
#else
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000L;
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
        /* continue sleeping */
    }
#endif
}

static void ensure_directory_exists(const char *path)
{
    if (!path) {
        return;
    }

    char dir[512];
    strncpy(dir, path, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';

    char *last_slash = strrchr(dir, '/');
#if defined(_WIN32)
    char *last_bslash = strrchr(dir, '\\');
    if (last_bslash && (!last_slash || last_bslash > last_slash)) {
        last_slash = last_bslash;
    }
#endif

    if (last_slash) {
        *last_slash = '\0';
        if (strlen(dir) > 0) {
#if defined(_WIN32)
            _mkdir(dir);
#else
            mkdir(dir, 0755);
#endif
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Queue Management Implementation                                   */
/* ------------------------------------------------------------------ */

bool log_queue_init(LogQueue *q, int capacity)
{
    if (!q || capacity <= 0) {
        CC_ERROR("log_queue_init: invalid arguments (q=%p, capacity=%d)",
                 (void *)q, capacity);
        return false;
    }

    q->buffer = (LogEvent *)calloc((size_t)capacity, sizeof(LogEvent));
    if (!q->buffer) {
        CC_ERROR("log_queue_init: failed to allocate memory for %d events: %s",
                 capacity, strerror(errno));
        return false;
    }

    q->capacity = capacity;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    q->producers_done = false;
    q->active_producers = 0;

    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        CC_ERROR("log_queue_init: pthread_mutex_init failed: %s", strerror(errno));
        free(q->buffer);
        q->buffer = NULL;
        return false;
    }

    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        CC_ERROR("log_queue_init: pthread_cond_init(not_full) failed: %s", strerror(errno));
        pthread_mutex_destroy(&q->mutex);
        free(q->buffer);
        q->buffer = NULL;
        return false;
    }

    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        CC_ERROR("log_queue_init: pthread_cond_init(not_empty) failed: %s", strerror(errno));
        pthread_cond_destroy(&q->not_full);
        pthread_mutex_destroy(&q->mutex);
        free(q->buffer);
        q->buffer = NULL;
        return false;
    }

    return true;
}

void log_queue_destroy(LogQueue *q)
{
    if (!q) {
        return;
    }

    pthread_mutex_lock(&q->mutex);
    q->producers_done = true;
    pthread_mutex_unlock(&q->mutex);

    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);

    if (q->buffer) {
        free(q->buffer);
        q->buffer = NULL;
    }

    q->capacity = 0;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    q->active_producers = 0;
}

void log_queue_enqueue(LogQueue *q, LogEvent *event)
{
    if (!q || !event) {
        CC_ERROR("log_queue_enqueue: invalid NULL parameter");
        return;
    }

    pthread_mutex_lock(&q->mutex);

    while (q->count == q->capacity) {
        printf("[Producer %d (TID %lu)] Queue FULL (%d/%d) - blocking on not_full condition...\n",
               event->producer_id, (unsigned long)pthread_self(), q->count, q->capacity);
        fflush(stdout);
        pthread_cond_wait(&q->not_full, &q->mutex);
    }

    q->buffer[q->tail] = *event;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;

    pthread_cond_signal(&q->not_empty);

    pthread_mutex_unlock(&q->mutex);
}

bool log_queue_dequeue(LogQueue *q, LogEvent *event)
{
    if (!q || !event) {
        CC_ERROR("log_queue_dequeue: invalid NULL parameter");
        return false;
    }

    pthread_mutex_lock(&q->mutex);

    while (q->count == 0 && !q->producers_done) {
        printf("[Consumer (TID %lu)] Queue EMPTY (%d/%d) - waiting on not_empty condition...\n",
               (unsigned long)pthread_self(), q->count, q->capacity);
        fflush(stdout);
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }

    if (q->count == 0 && q->producers_done) {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }

    *event = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    pthread_cond_signal(&q->not_full);

    pthread_mutex_unlock(&q->mutex);
    return true;
}

/* ------------------------------------------------------------------ */
/*  Thread Functions                                                  */
/* ------------------------------------------------------------------ */

void *producer_thread(void *arg)
{
    if (!arg) {
        return NULL;
    }

    ProducerArg *p_arg = (ProducerArg *)arg;
    LogQueue *q = p_arg->queue;
    int pid = p_arg->producer_id;
    int num_events = p_arg->events_per_producer;

    printf("[Producer %d (TID %lu)] Started: generating %d events\n",
           pid, (unsigned long)pthread_self(), num_events);
    fflush(stdout);

    for (int i = 1; i <= num_events; i++) {
        LogEvent ev;
        memset(&ev, 0, sizeof(ev));
        ev.producer_id = pid;
        ev.event_id = i;

        time_t now = time(NULL);
        struct tm tm_info;
        char time_str[32];
#if defined(_POSIX_C_SOURCE) || defined(__linux__) || defined(__unix__)
        localtime_r(&now, &tm_info);
#else
        localtime_s(&tm_info, &now);
#endif
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

        snprintf(ev.message, sizeof(ev.message),
                 "Batch task #%d processed on campus compute node %d",
                 i, pid);

        log_queue_enqueue(q, &ev);

        printf("[Producer %d (TID %lu)] Enqueued event #%d: \"%s\"\n",
               pid, (unsigned long)pthread_self(), i, ev.message);
        fflush(stdout);

        brief_pause_ms(5);
    }

    /* Producer finished all its events; update active producers */
    pthread_mutex_lock(&q->mutex);
    q->active_producers--;
    printf("[Producer %d (TID %lu)] Finished all %d events (active producers left: %d)\n",
           pid, (unsigned long)pthread_self(), num_events, q->active_producers);
    fflush(stdout);

    if (q->active_producers == 0) {
        q->producers_done = true;
        printf("[Producer %d (TID %lu)] Last producer finished; setting producers_done=true and broadcasting not_empty\n",
               pid, (unsigned long)pthread_self());
        fflush(stdout);
        pthread_cond_broadcast(&q->not_empty);
    }
    pthread_mutex_unlock(&q->mutex);

    return NULL;
}

void *consumer_thread(void *arg)
{
    if (!arg) {
        return NULL;
    }

    ConsumerArg *c_arg = (ConsumerArg *)arg;
    LogQueue *q = c_arg->queue;
    FILE *fp = c_arg->output_file;
    c_arg->events_consumed = 0;

    printf("[Consumer (TID %lu)] Started consumer thread\n", (unsigned long)pthread_self());
    fflush(stdout);

    LogEvent ev;
    while (log_queue_dequeue(q, &ev)) {
        c_arg->events_consumed++;

        time_t now = time(NULL);
        struct tm tm_info;
        char time_str[32];
#if defined(_POSIX_C_SOURCE) || defined(__linux__) || defined(__unix__)
        localtime_r(&now, &tm_info);
#else
        localtime_s(&tm_info, &now);
#endif
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

        /* Log file format: one line per event with timestamp, producer ID, event number, and message */
        fprintf(fp, "[%s] Producer: %d | Event: %d | %s\n",
                time_str, ev.producer_id, ev.event_id, ev.message);
        fflush(fp);

        printf("[Consumer (TID %lu)] Dequeued and logged event #%d from Producer %d (total consumed: %d)\n",
               (unsigned long)pthread_self(), ev.event_id, ev.producer_id, c_arg->events_consumed);
        fflush(stdout);

        brief_pause_ms(8);
    }

    printf("[Consumer (TID %lu)] Finished: consumer exiting (total events consumed: %d)\n",
           (unsigned long)pthread_self(), c_arg->events_consumed);
    fflush(stdout);

    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Runner Implementation                                             */
/* ------------------------------------------------------------------ */

int logger_run(LoggerConfig *config)
{
    int num_producers = 2;
    int events_per_producer = 5;
    int queue_capacity = 5;
    const char *output_path = "logs/campuscompute.log";

    if (config) {
        if (config->num_producers > 0) {
            num_producers = config->num_producers;
        }
        if (config->events_per_producer > 0) {
            events_per_producer = config->events_per_producer;
        }
        if (config->queue_capacity > 0) {
            queue_capacity = config->queue_capacity;
        }
        if (config->output_path && config->output_path[0] != '\0') {
            output_path = config->output_path;
        }
    }

    print_section("Concurrent Activity Logger (Producer-Consumer)");
    printf("Configuration:\n");
    printf("  Producers:            %d\n", num_producers);
    printf("  Events per Producer:  %d\n", events_per_producer);
    printf("  Total Events:         %d\n", num_producers * events_per_producer);
    printf("  Queue Capacity:       %d\n", queue_capacity);
    printf("  Output File Path:     %s\n", output_path);
    print_separator();

    ensure_directory_exists(output_path);

    FILE *fp = fopen(output_path, "w");
    if (!fp) {
        CC_ERROR("logger_run: failed to open output file '%s': %s",
                 output_path, strerror(errno));
        return -1;
    }

    LogQueue queue;
    if (!log_queue_init(&queue, queue_capacity)) {
        CC_ERROR("logger_run: failed to initialize log queue");
        fclose(fp);
        return -1;
    }

    queue.active_producers = num_producers;

    /* Start consumer thread */
    pthread_t consumer_tid;
    ConsumerArg c_arg;
    c_arg.queue = &queue;
    c_arg.output_file = fp;
    c_arg.events_consumed = 0;

    if (pthread_create(&consumer_tid, NULL, consumer_thread, &c_arg) != 0) {
        CC_ERROR("logger_run: failed to create consumer thread: %s", strerror(errno));
        fclose(fp);
        log_queue_destroy(&queue);
        return -1;
    }

    /* Start producer threads */
    pthread_t *producer_tids = (pthread_t *)safe_malloc(sizeof(pthread_t) * (size_t)num_producers);
    ProducerArg *producer_args = (ProducerArg *)safe_malloc(sizeof(ProducerArg) * (size_t)num_producers);

    int launched_producers = 0;
    for (int i = 0; i < num_producers; i++) {
        producer_args[i].queue = &queue;
        producer_args[i].producer_id = i + 1;
        producer_args[i].events_per_producer = events_per_producer;

        if (pthread_create(&producer_tids[i], NULL, producer_thread, &producer_args[i]) != 0) {
            CC_ERROR("logger_run: failed to create producer thread %d: %s",
                     i + 1, strerror(errno));
            pthread_mutex_lock(&queue.mutex);
            queue.active_producers--;
            if (queue.active_producers == 0) {
                queue.producers_done = true;
                pthread_cond_broadcast(&queue.not_empty);
            }
            pthread_mutex_unlock(&queue.mutex);
        } else {
            launched_producers++;
        }
    }

    /* Wait for all launched producer threads */
    for (int i = 0; i < launched_producers; i++) {
        pthread_join(producer_tids[i], NULL);
    }
    printf("[Logger Main] All %d producer thread(s) have joined.\n", launched_producers);
    fflush(stdout);

    /* Wait for consumer thread to drain queue and finish */
    pthread_join(consumer_tid, NULL);
    printf("[Logger Main] Consumer thread has joined.\n");
    fflush(stdout);

    /* Close file and destroy queue */
    fclose(fp);
    log_queue_destroy(&queue);
    free(producer_tids);
    free(producer_args);

    print_separator();
    printf("Logger Run Summary:\n");
    printf("  Producers launched:   %d\n", launched_producers);
    printf("  Events / producer:    %d\n", events_per_producer);
    printf("  Total events logged:  %d\n", c_arg.events_consumed);
    printf("  Log file location:    %s\n", output_path);
    printf("  Status:               SUCCESS - clean shutdown\n");
    print_separator();

    return 0;
}
