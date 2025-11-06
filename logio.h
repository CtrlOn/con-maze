#ifndef LOGIO_H
#define LOGIO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

/* Minimal header-only logging utility.
   Public functions:
     - log_start(const char *path)    : initialize logging (optional)
     - log_info(const char *fmt, ...) : informational message
     - log_warn(const char *fmt, ...) : warning message
     - log_error(const char *fmt, ...) : error message

   At exit the library writes runtime length (seconds) and closes the log file.
*/

#ifndef LOGIO_DEFAULT_PATH
#define LOGIO_DEFAULT_PATH "runtime.log"
#endif

static FILE *logio_file = NULL;
static time_t logio_start_time = 0;
static int logio_initialized = 0;

/* Internal: format current time into buffer (localtime). */
static void logio_now_str(char *buf, size_t bufsz) {
    time_t t = time(NULL);
    struct tm tm_buf;
    struct tm *tm_ptr = localtime(&t); /* portable fallback; copy into local buffer */

    if (tm_ptr) {
        tm_buf = *tm_ptr;
    } else {
        /* if localtime failed, zero the struct to produce a predictable timestamp */
        memset(&tm_buf, 0, sizeof(tm_buf));
    }

    strftime(buf, bufsz, "%Y-%m-%d %H:%M:%S", &tm_buf);
}

/* Close log and write runtime summary. Registered with atexit. */
static void logio_close(void) {
    if (!logio_initialized) return;
    time_t end = time(NULL);
    double seconds = difftime(end, logio_start_time);

    char ts[64];
    logio_now_str(ts, sizeof(ts));

    if (logio_file) {
        fprintf(logio_file, "[%s] LOG: shutdown\n", ts);
        fprintf(logio_file, "Runtime: %.0f seconds\n", seconds);
        fflush(logio_file);
        fclose(logio_file);
        logio_file = NULL;
    }

    logio_initialized = 0;
}

/* Initialize logging to a given path. Safe to call multiple times. */
static void log_start(const char *path) {
    if (logio_initialized) return;

    logio_file = fopen(path ? path : LOGIO_DEFAULT_PATH, "a");
    logio_start_time = time(NULL);
    logio_initialized = 1;

    char ts[64];
    logio_now_str(ts, sizeof(ts));
    if (logio_file) {
        fprintf(logio_file, "[%s] LOG: start\n", ts);
        fflush(logio_file);
    }

    /* ensure summary is written at program exit */
    atexit(logio_close);
}

/* Lazy init: called by log functions if user didn't call log_start. */
static void logio_ensure_init(void) {
    if (!logio_initialized) log_start(NULL);
}

/* Core logging function */
static void logio_logv(const char *level, const char *fmt, va_list ap) {
    logio_ensure_init();

    if (!logio_file) {
        /* if file isn't available, do nothing (no console output) */
        return;
    }

    char ts[64];
    logio_now_str(ts, sizeof(ts));

    /* print to file only */
    fprintf(logio_file, "[%s] %s: ", ts, level);
    vfprintf(logio_file, fmt, ap);
    fprintf(logio_file, "\n");
    fflush(logio_file);
}

/* Public logging helpers (vararg wrappers) */
static void log_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logio_logv("INFO", fmt, ap);
    va_end(ap);
}

static void log_warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logio_logv("WARN", fmt, ap);
    va_end(ap);
}

static void log_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logio_logv("ERROR", fmt, ap);
    va_end(ap);
}

#endif /* LOGIO_H */