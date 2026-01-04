#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "binio.h"
#include "savesdir.h"
#include "loglib.h"

int localDataLoaded = 0;
int levelCount = 0;
char** levelNames = NULL;
int* finishedGameCounts = NULL;
char*** finishedPlayerNames = NULL;
int** finishedMovesCounts = NULL;
int* ongoingGameCounts = NULL;
char*** ongoingPlayerNames = NULL;

void freeLocalData(void) {
    if (!localDataLoaded) return;

    if (levelNames) {
        for (int i = 0; i < levelCount; i++) {
            if (levelNames[i]) free(levelNames[i]);
        }
        free(levelNames);
        levelNames = NULL;
    }

    if (finishedPlayerNames) {
        for (int i = 0; i < levelCount; i++) {
            if (finishedPlayerNames[i]) {
                int count = finishedGameCounts ? finishedGameCounts[i] : 0;

                for (int j = 0; j < count; j++) {
                    if (finishedPlayerNames[i][j]) {
                        free(finishedPlayerNames[i][j]);
                    }
                }

                free(finishedPlayerNames[i]);
            }
        }
        free(finishedPlayerNames);
        finishedPlayerNames = NULL;
    }

    if (finishedMovesCounts) {
        for (int i = 0; i < levelCount; i++) {
            if (finishedMovesCounts[i]) {
                free(finishedMovesCounts[i]);
            }
        }
        free(finishedMovesCounts);
        finishedMovesCounts = NULL;
    }

    if (finishedGameCounts) {
        free(finishedGameCounts);
        finishedGameCounts = NULL;
    }

    if (ongoingPlayerNames) {
        for (int i = 0; i < levelCount; i++) {
            if (ongoingPlayerNames[i]) {
                int count = ongoingGameCounts ? ongoingGameCounts[i] : 0;

                for (int j = 0; j < count; j++) {
                    if (ongoingPlayerNames[i][j]) {
                        free(ongoingPlayerNames[i][j]);
                    }
                }

                free(ongoingPlayerNames[i]);
            }
        }
        free(ongoingPlayerNames);
        ongoingPlayerNames = NULL;
    }

    if (ongoingGameCounts) {
        free(ongoingGameCounts);
        ongoingGameCounts = NULL;
    }

    localDataLoaded = 0;
    levelCount = 0;
}

void fetchLocalData(void) {
    if (localDataLoaded) {
        freeLocalData();
    }

    DIR *dir = opendir(LEVELS_FOLDER);
    if (!dir) {
        log_error("Failed to open levels directory '%s'.", LEVELS_FOLDER);
        levelCount = 0;
        localDataLoaded = 1;
        return;
    }

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        count++;
    }
    closedir(dir);

    levelCount = count;

    levelNames = calloc(levelCount, sizeof(char*));
    finishedGameCounts = calloc(levelCount, sizeof(int));
    finishedPlayerNames = calloc(levelCount, sizeof(char**));
    finishedMovesCounts = calloc(levelCount, sizeof(int*));
    ongoingGameCounts = calloc(levelCount, sizeof(int));
    ongoingPlayerNames = calloc(levelCount, sizeof(char**));

    dir = opendir(LEVELS_FOLDER);
    int idx = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char* name = strdup(entry->d_name);
        char* dot = strrchr(name, '.');
        if (dot && strcmp(dot, ".dat") == 0) *dot = '\0';
        levelNames[idx] = name;

        char finishedPath[256];
        sprintf(finishedPath, GAMES_FOLDER"/%s/"FINISHED_FOLDER"/", name);

        DIR *fdir = opendir(finishedPath);
        if (fdir) {
            int fcount = 0;
            struct dirent *fentry;
            while ((fentry = readdir(fdir)) != NULL) {
                if (fentry->d_name[0] == '.') continue;
                fcount++;
            }
            closedir(fdir);

            finishedGameCounts[idx] = fcount;
            finishedPlayerNames[idx] = calloc(fcount, sizeof(char*));
            finishedMovesCounts[idx] = calloc(fcount, sizeof(int));

            fdir = opendir(finishedPath);
            int fidx = 0;
            while ((fentry = readdir(fdir)) != NULL) {
                if (fentry->d_name[0] == '.') continue;

                char* saveName = strdup(fentry->d_name);
                char* dot = strrchr(saveName, '.');
                if (dot && strcmp(dot, ".bin") == 0) *dot = '\0';

                finishedPlayerNames[idx][fidx] = saveName;

                char fullPath[261]; // 256 + 5
                sprintf(fullPath, "%s/%s.bin", finishedPath, saveName);

                int loadedMoves;
                char* loadedSeq;
                if (loadData(fullPath, &loadedMoves, &loadedSeq)) {
                    finishedMovesCounts[idx][fidx] = loadedMoves;
                    free(loadedSeq);
                } else {
                    finishedMovesCounts[idx][fidx] = -1;
                    log_error("Failed to load moves for %s", fullPath);
                }

                fidx++;
            }
            closedir(fdir);
        }

        char ongoingPath[256];
        sprintf(ongoingPath, GAMES_FOLDER"/%s/"ONGOING_FOLDER"/", name);

        DIR *odir = opendir(ongoingPath);
        if (odir) {
            int ocount = 0;
            struct dirent *oentry;
            while ((oentry = readdir(odir)) != NULL) {
                if (oentry->d_name[0] == '.') continue;
                ocount++;
            }
            closedir(odir);

            ongoingGameCounts[idx] = ocount;
            ongoingPlayerNames[idx] = calloc(ocount, sizeof(char*));

            odir = opendir(ongoingPath);
            int oidx = 0;

            while ((oentry = readdir(odir)) != NULL) {
                if (oentry->d_name[0] == '.') continue;

                char* saveName = strdup(oentry->d_name);
                char* dot = strrchr(saveName, '.');
                if (dot && strcmp(dot, ".bin") == 0) *dot = '\0';

                ongoingPlayerNames[idx][oidx] = saveName;

                char fullPath[261]; // 256 + 5
                sprintf(fullPath, "%s/%s.bin", ongoingPath, saveName);

                int loadedMoves;
                char* loadedSeq;
                if (loadData(fullPath, &loadedMoves, &loadedSeq)) {
                    free(loadedSeq);
                } else {
                    log_error("Failed to load moves for %s", fullPath);
                }

                oidx++;
            }

            closedir(odir);
        }

        idx++;
    }

    closedir(dir);
    localDataLoaded = 1;
}
