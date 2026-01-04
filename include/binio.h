#ifndef BINIO_H
#define BINIO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "loglib.h"
#ifdef _WIN32
#include <direct.h>
#define DIR_SEP "\\"
#else
#include <sys/stat.h>
#define DIR_SEP "/"
#endif

// Function declarations
void createDirectories(const char *path);
int findData(const char *path);
int saveData(const char *path, int count, const char *data);
int loadData(const char *path, int *out_count, char **out_data);
int deleteData(const char *path);

#endif // BINIO_H
