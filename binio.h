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

static inline void createDirectories(const char *path) {
    char *pathCopy = strdup(path);
    char *p = pathCopy;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
#ifdef _WIN32
            _mkdir(pathCopy);
#else
            mkdir(pathCopy, 0755);
#endif
            *p = DIR_SEP[0];
        }
        p++;
    }
    // Create the final directory if path doesn't end with separator
    if (strlen(pathCopy) > 0) {
#ifdef _WIN32
        _mkdir(pathCopy);
#else
        mkdir(pathCopy, 0755);
#endif
    }
    free(pathCopy);
}

static inline int findData(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

static inline int saveData(const char *path, int count, const char *data) {
    // Extract directory path
    char *pathCopy = strdup(path);
    char *lastSep = strrchr(pathCopy, '/');
    if (!lastSep) lastSep = strrchr(pathCopy, '\\');
    if (lastSep) {
        *lastSep = '\0';
        createDirectories(pathCopy);
    }
    free(pathCopy);
    FILE *file = fopen(path, "wb");
    if (!file) {
        log_error("Failed to open file for writing: %s (errno: %d)", path, errno);
        return 0;
    }
    if (fwrite(&count, sizeof(int), 1, file) != 1) {
        log_error("Failed to write count to file: %s", path);
        fclose(file);
        return 0;
    }
    if (count > 0 && data) {
        if (fwrite(data, sizeof(char), (size_t)count, file) != (size_t)count) {
            log_error("Failed to write data to file: %s", path);
            fclose(file);
            return 0;
        }
    }
    fclose(file);
    log_info("Successfully saved data to %s", path);
    return 1;
}

static inline int loadData(const char *path, int *out_count, char **out_data) {
    if (!out_count || !out_data) return 0;
    *out_count = 0;
    *out_data = NULL;

    FILE *file = fopen(path, "rb");
    if (!file) return 0;

    int loadedMoves = 0;
    if (fread(&loadedMoves, sizeof(int), 1, file) != 1) {
        fclose(file);
        return 0;
    }

    char *loadedSequence = NULL;
    if (loadedMoves > 0) {
        loadedSequence = (char*)malloc((size_t)loadedMoves * sizeof(char));
        if (!loadedSequence) {
            fclose(file);
            return 0;
        }
        if (fread(loadedSequence, sizeof(char), (size_t)loadedMoves, file) != (size_t)loadedMoves) {
            free(loadedSequence);
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    *out_count = loadedMoves;
    *out_data = loadedSequence;
    return 1;
}

static inline int deleteData(const char *path) {
    return (remove(path) == 0) ? 1 : 0;
}

#endif // BINIO_H