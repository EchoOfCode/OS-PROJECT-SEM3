/*
 * common.h - Common utilities for CampusCompute
 *
 * Provides error reporting, safe memory allocation, and safe
 * string-to-integer conversion functions used across all modules.
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>
#include <stdarg.h>

/* Error and info reporting functions and macros */
void cc_log_error(const char *fmt, ...);
void cc_log_info(const char *fmt, ...);
void cc_log_warn(const char *fmt, ...);

#define CC_ERROR(...) cc_log_error(__VA_ARGS__)
#define CC_INFO(...) cc_log_info(__VA_ARGS__)
#define CC_WARN(...) cc_log_warn(__VA_ARGS__)

/* Safe memory allocation - exits on failure */
void *safe_malloc(size_t size);
void *safe_calloc(size_t nmemb, size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *s);

/* Safe string-to-integer conversion with full error checking */
bool safe_strtol(const char *str, long *out);
bool safe_strtoll(const char *str, long long *out);

/* Trim leading/trailing whitespace in-place, return pointer */
char *str_trim(char *s);

/* Print a separator line */
void print_separator(void);

/* Print a section header */
void print_section(const char *title);

#endif /* COMMON_H */
