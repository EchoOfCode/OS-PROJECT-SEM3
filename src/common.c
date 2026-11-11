/*
 * common.c - Implementation of common utilities for CampusCompute
 *
 * See include/common.h for the public interface.
 */
#include "common.h"
#include <ctype.h>
#include <stdarg.h>

/* ------------------------------------------------------------------ */
/*  Logging and message output functions                              */
/* ------------------------------------------------------------------ */

void cc_log_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void cc_log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void cc_log_warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "WARNING: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/* ------------------------------------------------------------------ */
/*  Safe memory allocation wrappers                                   */
/* ------------------------------------------------------------------ */

void *safe_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr) {
        CC_ERROR("malloc(%zu) failed: %s", size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *safe_calloc(size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        CC_ERROR("calloc(%zu, %zu) failed: %s", nmemb, size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *safe_realloc(void *ptr, size_t size)
{
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size != 0) {
        CC_ERROR("realloc(%zu) failed: %s", size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

char *safe_strdup(const char *s)
{
    if (!s) {
        CC_ERROR("safe_strdup(NULL)");
        exit(EXIT_FAILURE);
    }
    char *dup = strdup(s);
    if (!dup) {
        CC_ERROR("strdup failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return dup;
}

/* ------------------------------------------------------------------ */
/*  Safe string-to-integer conversion                                 */
/* ------------------------------------------------------------------ */

bool safe_strtol(const char *str, long *out)
{
    if (!str || !out) {
        CC_ERROR("safe_strtol: NULL argument");
        return false;
    }

    /* Skip leading whitespace */
    while (isspace((unsigned char)*str))
        str++;

    if (*str == '\0') {
        CC_ERROR("safe_strtol: empty string");
        return false;
    }

    errno = 0;
    char *endptr = NULL;
    long val = strtol(str, &endptr, 10);

    if (errno == ERANGE) {
        CC_ERROR("safe_strtol: value out of range: '%s'", str);
        return false;
    }
    if (endptr == str) {
        CC_ERROR("safe_strtol: no digits found in '%s'", str);
        return false;
    }
    /* Reject trailing non-whitespace characters */
    while (isspace((unsigned char)*endptr))
        endptr++;
    if (*endptr != '\0') {
        CC_ERROR("safe_strtol: trailing characters in '%s'", str);
        return false;
    }

    *out = val;
    return true;
}

bool safe_strtoll(const char *str, long long *out)
{
    if (!str || !out) {
        CC_ERROR("safe_strtoll: NULL argument");
        return false;
    }

    while (isspace((unsigned char)*str))
        str++;

    if (*str == '\0') {
        CC_ERROR("safe_strtoll: empty string");
        return false;
    }

    errno = 0;
    char *endptr = NULL;
    long long val = strtoll(str, &endptr, 10);

    if (errno == ERANGE) {
        CC_ERROR("safe_strtoll: value out of range: '%s'", str);
        return false;
    }
    if (endptr == str) {
        CC_ERROR("safe_strtoll: no digits found in '%s'", str);
        return false;
    }
    while (isspace((unsigned char)*endptr))
        endptr++;
    if (*endptr != '\0') {
        CC_ERROR("safe_strtoll: trailing characters in '%s'", str);
        return false;
    }

    *out = val;
    return true;
}

/* ------------------------------------------------------------------ */
/*  String utilities                                                   */
/* ------------------------------------------------------------------ */

char *str_trim(char *s)
{
    if (!s)
        return NULL;

    /* Trim leading whitespace */
    while (isspace((unsigned char)*s))
        s++;

    if (*s == '\0')
        return s;

    /* Trim trailing whitespace */
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end))
        end--;
    *(end + 1) = '\0';

    return s;
}

/* ------------------------------------------------------------------ */
/*  Formatted output helpers                                           */
/* ------------------------------------------------------------------ */

void print_separator(void)
{
    printf("=========================================="
           "==========================================\n");
}

void print_section(const char *title)
{
    printf("\n");
    print_separator();
    if (title) {
        printf("  %s\n", title);
    }
    print_separator();
}
