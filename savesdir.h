#ifndef SAVESDIR_H
#define SAVESDIR_H

#include <stddef.h>

#define GAMES_FOLDER "saves/games"
#define FINISHED_FOLDER "finished"
#define ONGOING_FOLDER "ongoing"
#define LEVELS_FOLDER "saves/levels"

extern int localDataLoaded;
extern int levelCount;
extern char** levelNames;
extern int* finishedGameCounts;
extern char*** finishedPlayerNames;
extern int** finishedMovesCounts;
extern int* ongoingGameCounts;
extern char*** ongoingPlayerNames;

void freeLocalData(void);
void fetchLocalData(void);

#endif // SAVESDIR_H
