#include "loglib.h"
#include <string.h>

FILE *loglib_file = NULL;
time_t loglib_start_time = 0;
int loglib_initialized = 0;

// Internal: format current time into buffer (localtime).
static void loglib_now_str(char *buf, size_t bufsz) {
    time_t t = time(NULL);
    struct tm tm_buf;
    struct tm *tm_ptr = localtime(&t); // portable fallback; copy into local buffer

    if (tm_ptr) {
        tm_buf = *tm_ptr;
    } else {
        // if localtime failed, zero the struct to produce a predictable timestamp
        memset(&tm_buf, 0, sizeof(tm_buf));
    }

    strftime(buf, bufsz, "%Y-%m-%d %H:%M:%S", &tm_buf);
}

// Internal: generate log filename based on init time.
static void loglib_generate_filename(char *buf, size_t bufsz, time_t init_time) {
    struct tm tm_buf;
    struct tm *tm_ptr = localtime(&init_time);

    if (tm_ptr) {
        tm_buf = *tm_ptr;
    } else {
        memset(&tm_buf, 0, sizeof(tm_buf));
    }

    strftime(buf, bufsz, "logs/runtime_%Y-%m-%d_%H-%M-%S.log", &tm_buf);
}

// Close log and write runtime summary. Registered with atexit.
static void loglib_close(void) {
    if (!loglib_initialized) return;
    time_t end = time(NULL);
    double seconds = difftime(end, loglib_start_time);

    char ts[64];
    loglib_now_str(ts, sizeof(ts));

    if (loglib_file) {
        fprintf(loglib_file, "[%s] LOG: shutdown\n", ts);
        fprintf(loglib_file, "Runtime: %.0f seconds\n", seconds);
        fflush(loglib_file);
        fclose(loglib_file);
        loglib_file = NULL;
    }

    loglib_initialized = 0;
}

// Initialize logging to a given path. Safe to call multiple times.
void log_start_path(const char *path) {
    if (loglib_initialized) return;

    char filepath[256];
    loglib_start_time = time(NULL);
    if (path) {
        strcpy(filepath, path);
        loglib_file = fopen(filepath, "a"); // append for custom paths
    } else {
        loglib_generate_filename(filepath, sizeof(filepath), loglib_start_time);
#ifdef _WIN32
        _mkdir("logs"); // create logs directory if it doesn't exist
#else
        mkdir("logs", 0755); // create logs directory if it doesn't exist
#endif
        loglib_file = fopen(filepath, "w"); // write mode for new file per session
    }
    loglib_initialized = 1;

    char ts[64];
    loglib_now_str(ts, sizeof(ts));
    if (loglib_file) {
        fprintf(loglib_file, "[%s] LOG: start\n", ts);
        fflush(loglib_file);
    }

    // ensure summary is written at program exit
    atexit(loglib_close);
}

// Initialize logging with default behavior.
void log_start(void) {
    log_start_path(NULL);
}

// Lazy init: called by log functions if user didn't call log_start.
static void loglib_ensure_init(void) {
    if (!loglib_initialized) log_start_path(NULL);
}

// Core logging function
static void loglib_logv(const char *level, const char *fmt, va_list ap) {
    loglib_ensure_init();

    if (!loglib_file) {
        // if file isn't available, do nothing (no console output)
        return;
    }

    char ts[64];
    loglib_now_str(ts, sizeof(ts));

    // print to file only
    fprintf(loglib_file, "[%s] %s: ", ts, level);
    vfprintf(loglib_file, fmt, ap);
    fprintf(loglib_file, "\n");
    fflush(loglib_file);
}

// Public logging helpers (vararg wrappers)
void log_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    loglib_logv("INFO", fmt, ap);
    va_end(ap);
}

void log_warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    loglib_logv("WARN", fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    loglib_logv("ERROR", fmt, ap);
    va_end(ap);
}
