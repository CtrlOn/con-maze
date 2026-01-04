#ifndef PLATFORM_H
#define PLATFORM_H

#ifndef _WIN32
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

#include <dirent.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

// ANSI escape color helper
#define ANSI_COL(text, code) "\x1B[" code "m" text "\x1B[0m"

// Cursor visibility helpers
#define HIDE_CURSOR() printf("\033[?25l")
#define SHOW_CURSOR() printf("\033[?25h")

// Map to platform functions so code can use the macro style
#define CLEAR_SCREEN() platform_clear_screen()
#define HOME_CURSOR() platform_home_cursor()

char getch_portable(void);
void flushInput(void);
void platform_clear_screen(void);
void platform_home_cursor(void);

#ifdef _WIN32
int usleep(unsigned int usec); // implemented for Windows
#endif

#endif // PLATFORM_H
