#ifndef LOGLIB_H
#define LOGLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

// External variable declarations
extern FILE *loglib_file;
extern time_t loglib_start_time;
extern int loglib_initialized;

// Function declarations
void log_start_path(const char *path);
void log_start(void);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);

#endif // LOGLIB_H
