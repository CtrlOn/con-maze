#include "platform.h"
#include "binio.h"
#include "loglib.h"

#define GAMES_FOLDER "saves/games"
#define FINISHED_FOLDER "finished"
#define ONGOING_FOLDER "ongoing"
#define LEVELS_FOLDER "saves/levels"


#define ASCII_LOGO \
ANSI_COL("   ######    #######   ####     ##", "96")ANSI_COL("      ", "97")ANSI_COL(" ####     ####     ##     ######## ########\n", "94") \
ANSI_COL("  ##////##  ##/////## /##/##   /##", "96")ANSI_COL("      ", "97")ANSI_COL("/##/##   ##/##    ####   //////## /##///// \n", "94") \
ANSI_COL(" ##    //  ##     //##/##//##  /##", "96")ANSI_COL("      ", "97")ANSI_COL("/##//## ## /##   ##//##       ##  /##      \n", "94") \
ANSI_COL("/##       /##      /##/## //## /##", "96")ANSI_COL(" #####", "97")ANSI_COL("/## //###  /##  ##  //##     ##   /####### \n", "94") \
ANSI_COL("/##       /##      /##/##  //##/##", "96")ANSI_COL("///// ", "97")ANSI_COL("/##  //#   /## ##########   ##    /##////  \n", "94") \
ANSI_COL("//##    ##//##     ## /##   //####", "96")ANSI_COL("      ", "97")ANSI_COL("/##   /    /##/##//////##  ##     /##      \n", "94") \
ANSI_COL(" //######  //#######  /##    //###", "96")ANSI_COL("      ", "97")ANSI_COL("/##        /##/##     /## ########/########\n", "94") \
ANSI_COL("  //////    ///////   //      /// ", "96")ANSI_COL("      ", "97")ANSI_COL("//         // //      // //////// //////// \n", "94") 

#define UP [playerR][playerY - 1][playerX]
#define DOWN [playerR][playerY + 1][playerX]
#define LEFT [playerR][playerY][playerX - 1]
#define RIGHT [playerR][playerY][playerX + 1]
#define HERE [playerR][playerY][playerX]

// ------------------------------------------------------------------------------------------------
// SYMBOL       METADATA    TILE        INFO
// ------------------------------------------------------------------------------------------------
// '#'          No          Wall        Impassable
// '&'          Yes         Door        Impassable until unlock
// '!'          Yes         Key         Collectible, unlocks doors of same ID
// ' '          No          Void        Passable
// '^'          Yes         Passage     Sends player to passage with same ID (must be 2 per ID)
// '@'          No          Start       Player start, passable
// '$'          No          Goal        Victory upon reaching
// a-zA-Z1-9    No          Text        Display only, passable
// ------------------------------------------------------------------------------------------------
// ID's are placed in metadata field, beyond the right side of room, ORDER MATTERS!
// Doors, keys, start has residue details (they act like Text)
// When player enters passage, it appears on top of next passage, to go back it will need to step off and enter back, make sure to leave room
// Do not let player escape map, case unhandled

// Symbols (chars)
#define CHAR_WALL          '#'
#define CHAR_DOOR          '&'
#define CHAR_KEY           '!'
#define CHAR_VOID          ' '
#define CHAR_PASSAGE       '^'
#define CHAR_START         '@'
#define CHAR_GOAL          '$'
#define HAS_METADATA(c) (c == CHAR_DOOR || c == CHAR_KEY || c == CHAR_PASSAGE)

// Tiles (2 characters wide for font justification)
// Get color codes from here https://i.sstatic.net/9UVnC.png
#define TILE_PLAYER         ANSI_COL("<>", "91;41")
#define TILE_START_RESIDUE  ANSI_COL("[]", "90;40")
#define TILE_VOID           ANSI_COL("  ", "90;40")
#define TILE_WALL           ANSI_COL("##", "37;100")
#define TILE_DOOR           ANSI_COL("##", "96;46")
#define TILE_DOOR_RESIDUE   ANSI_COL("##", "90;40") // Shows up when door is open (id = -1)
#define TILE_KEY            ANSI_COL("o+", "96;40")
#define TILE_KEY_RESIDUE    ANSI_COL("__", "90;40") // Shows up when key is collected (id = -1)
#define TILE_GOAL           ANSI_COL("[]", "93;43")
#define TILE_PASSAGE        ANSI_COL("[]", "93;42")
#define TILE_ERROR          ANSI_COL("??", "30;105") // Shows up when flagged (id = -2)
#define TILE_SYMBOL         ANSI_COL("%c ", "90;40") // For text symbols

// App state variables
int quitting = 0; // did user quit app through GUI
int atMenuGUI = 0; // is user in main menu?
int isGameLoaded = 0; // is a game running?
char* loadedLevelName;

// GUI state variables
int choicesGUI = 0; // how many choices are present in current GUI
int cursorGUI = 1; // which selection is user at
int submitGUI = 0; // is user submission pending?

// Game state variables
int roomWidth;
int roomCount;
char*** map = NULL;
int*** metadata = NULL;
int playerX;
int playerY;
int playerR;
int movesMade = 0;
char *moveSequence = NULL;

// Game state flags
int victory = 0;
int loading = 0;

// Locally loaded files
int localDataLoaded = 0;
int levelCount = 0;
char** levelNames = NULL;
int* finishedGameCounts = NULL;
char*** finishedPlayerNames = NULL;
int** finishedMovesCounts = NULL;
int* ongoingGameCounts = NULL;
char*** ongoingPlayerNames = NULL;

char* getStringInput(char* prompt) {
    printf("%s", prompt);
    char* buffer = (char*)malloc(64 * sizeof(char));
    int i = 0;
    while (1) {
        char c = getch_portable();
        if (c == '\r' || c == '\n') {
            buffer[i] = '\0';
            break;
        } else if (c == 8 || c == 127) { // backspace
            if (i > 0) {
                i--;
                printf("\b \b");
            }
        } else if (isprint((unsigned char)c) && i < 63) {
            buffer[i++] = c;
            printf("%c", c);
        }
    }
    printf("\n");
    return buffer;
}

void unloadGame() {
    // Free map
    if (map) {
        for (int r = 0; r < roomCount; ++r) {
            if (map[r]) {
                for (int i = 0; i < roomWidth; ++i) {
                    if (map[r][i]) {
                        free(map[r][i]);
                    }
                }
                free(map[r]);
            }
        }
        free(map);
        map = NULL;
    }

    // Free metadata
    if (metadata) {
        for (int r = 0; r < roomCount; ++r) {
            if (metadata[r]) {
                for (int i = 0; i < roomWidth; ++i) {
                    if (metadata[r][i]) {
                        free(metadata[r][i]);
                    }
                }
                free(metadata[r]);
            }
        }
        free(metadata);
        metadata = NULL;
    }

    // Free moveSequence
    if (moveSequence) {
        free(moveSequence);
        moveSequence = NULL;
    }

    // Reset game state variables
    roomWidth = 0;
    roomCount = 0;
    playerX = 0;
    playerY = 0;
    playerR = 0;
    movesMade = 0;
    victory = 0;
    loading = 0;
    isGameLoaded = 0;
    if (loadedLevelName)
        free(loadedLevelName);
    loadedLevelName = NULL;
}

void loadGame(char* levelFile) {
    if (isGameLoaded){
        log_warn("Game unloaded (lazy).");
        unloadGame();
    }
    char fullPath[256];
    sprintf(fullPath, "%s/%s.dat", LEVELS_FOLDER, levelFile);
    log_info("Loading level from %s", fullPath);
    FILE *f = fopen(fullPath, "r");
    if (!f) {
        log_error("Failed to open level file '%s'.", fullPath);
        goto cleanup;
    }

    char line[1024];
    int width = 0;
    // Find WIDTH
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == ';' || *p == '\0' || *p == '\n' || *p == '\r') continue;
        if (strncmp(p, "WIDTH", 5) == 0) {
            p += 5;
            while (*p && isspace((unsigned char)*p)) p++;
            width = atoi(p);
            break;
        }
        // Parse integer
        char *endptr;
        long val = strtol(p, &endptr, 10);
        if (endptr != p) {
            width = (int)val;
            break;
        }
    }
    if (width <= 0) {
        log_error("Missing or invalid WIDTH in level file.");
        fclose(f);
        goto cleanup;
    }
    if (width > 32) {
        log_warn("Rooms of high width might not fit in console window.");
    }
    roomWidth = width;

    // Find BEGIN marker
    int in_rooms = 0;
    char **tileLines = NULL;
    int tileCount = 0, tileCap = 0;
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        // Trim leading whitespace
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == ';') continue;
        // Trim trailing newline and whitespace
        char *end = p + strlen(p);
        while (end > p && (end[-1] == '\n' || end[-1] == '\r' || isspace((unsigned char)end[-1]))) end--;
        *end = '\0';
        if (!in_rooms) {
            if (strcmp(p, "BEGIN") == 0) {
                in_rooms = 1;
            }
            continue;
        } else {
            if (strcmp(p, "END") == 0) break;
            // Ignore empty lines
            if (*p == '\0') continue;
            if (tileCount == tileCap) {
                tileCap = tileCap ? tileCap * 2 : 64;
                tileLines = (char**)realloc(tileLines, tileCap * sizeof(char*));
            }
            tileLines[tileCount++] = strdup(p);
        }
    }
    fclose(f);

    if (tileCount == 0) {
        log_error("No room data found between BEGIN and END.");
        goto cleanup;
    }

    if (tileCount % roomWidth != 0) {
        log_warn("Number of tile lines (%zu) is not a multiple of WIDTH (%d). Truncating excess lines.", tileCount, roomWidth);
    }
    roomCount = (int)(tileCount / roomWidth);
    if (roomCount <= 0) {
        log_error("No valid rooms found in level data.");
        goto cleanup;
    }

    // Allocate map and metadata
    map = (char***)malloc(roomCount * sizeof(char**));
    if (!map) goto cleanup;
    metadata = (int***)malloc(roomCount * sizeof(int**));
    if (!metadata) goto cleanup;
    for (int r = 0; r < roomCount; ++r) {
        map[r] = (char**)malloc(roomWidth * sizeof(char*));
        if (!map[r]) goto cleanup;
        metadata[r] = (int**)malloc(roomWidth * sizeof(int*));
        if (!metadata[r]) goto cleanup;
        for (int i = 0; i < roomWidth; ++i) {
            map[r][i] = (char*)malloc(roomWidth * sizeof(char));
            if (!map[r][i]) goto cleanup;
            metadata[r][i] = (int*)malloc(roomWidth * sizeof(int));
            if (!metadata[r][i]) goto cleanup;
            for (int j = 0; j < roomWidth; ++j) metadata[r][i][j] = 0;
        }
    }

    // Parse rooms
    int foundStart = 0;
    int foundGoal = 0;
    for (int r = 0; r < roomCount; ++r) {
        // Collect metadata tokens for this room in order
        int *metaList = NULL;
        int metaCount = 0, metaCap = 0;

        for (int i = 0; i < roomWidth; ++i) {
            int idx = r * roomWidth + i;
            if (idx >= tileCount) break;
            char *lineptr = tileLines[idx];
            int linelen = strlen(lineptr);
            // first roomWidth characters are tile chars (pad with walls if short)
            int lowLength = 0;
            for (int j = 0; j < roomWidth; ++j) {
                if (j < linelen) {
                    map[r][i][j] = lineptr[j];
                } else {
                    map[r][i][j] = CHAR_WALL;
                    lowLength = 1;
                }
            }
            if (lowLength) {
                log_warn("Line %zu in room %d is shorter than WIDTH (%d). Padding with walls.", idx + 1, r, roomWidth);
            }
            // Parse trailing metadata tokens (if any) after first roomWidth chars
            if (linelen > roomWidth) {
                char *metaStart = lineptr + roomWidth;
                // Skip whitespace
                while (*metaStart && isspace((unsigned char)*metaStart)) metaStart++;
                char *tok = strtok(metaStart, " \t");
                while (tok) {
                    long v = strtol(tok, NULL, 10);
                    if (metaCount == metaCap) {
                        metaCap = metaCap ? metaCap * 2 : 8;
                        metaList = (int*)realloc(metaList, metaCap * sizeof(int));
                    }
                    metaList[metaCount++] = (int)v;
                    tok = strtok(NULL, " \t");
                }
            }
        }

        // Assign collected metadata sequentially to tiles that require metadata (row-major)
        int metaIndex = 0;
        for (int i = 0; i < roomWidth; ++i) {
            for (int j = 0; j < roomWidth; ++j) {
                char ch = map[r][i][j];
                if (HAS_METADATA(ch)) {
                    if (metaIndex < metaCount) {
                        metadata[r][i][j] = metaList[metaIndex++];
                    } else {
                        log_error("No metadata found for tile '%c' at (%d, %d) in room %d.", ch, j, i, r);
                        metadata[r][i][j] = -2;
                    }
                } else {
                    metadata[r][i][j] = -1;
                }
                // Locate start and goal tiles
                if (ch == CHAR_START) {
                    playerR = r;
                    playerY = i;
                    playerX = j;
                    foundStart++;
                }
                if (ch == CHAR_GOAL) {
                    foundGoal = 1;
                }
            }
            if (metaIndex > metaCount) {
                log_warn("Excess metadata at row %d in room %d.", i, r);
            }
        }
        free(metaList);
    }

    // validate overall level
    if (!foundStart) {
        log_error("No start tile '@' found in level data (any room).");
        goto cleanup;
    }
    if (foundStart > 1) {
        log_error("Multiple start tiles '@' found in level data (%d).", foundStart);
        goto cleanup;
    }
    if (!foundGoal) {
        log_warn("No goal tile '$' found in level data (any room).");
    }

    // Cleanup tileLines
    for (int i = 0; i < tileCount; ++i) free(tileLines[i]);
    free(tileLines);

    loadedLevelName = strdup(levelFile);
    if (!loadedLevelName) goto cleanup;

    isGameLoaded = 1;

    log_info("Loaded level: WIDTH=%d, ROOM_COUNT=%d", roomWidth, roomCount);
    return;

cleanup:
    if (map) {
        for (int r = 0; r < roomCount; ++r) {
            if (map[r]) {
                for (int i = 0; i < roomWidth; ++i) {
                    if (map[r][i]) free(map[r][i]);
                }
                free(map[r]);
            }
        }
        free(map);
        map = NULL;
    }
    if (metadata) {
        for (int r = 0; r < roomCount; ++r) {
            if (metadata[r]) {
                for (int i = 0; i < roomWidth; ++i) {
                    if (metadata[r][i]) free(metadata[r][i]);
                }
                free(metadata[r]);
            }
        }
        free(metadata);
        metadata = NULL;
    }
    if (loadedLevelName) free(loadedLevelName);
    loadedLevelName = NULL;
    roomWidth = 0;
    roomCount = 0;
    exit(1);
}

void freeLocalData() {
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
}

void fetchLocalData() {
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

void addMoveToSequence(char move) { // For faster loading, sys calls every 10 moves
    if (movesMade == 0) {
        moveSequence = (char*)malloc(10 * sizeof(char));
    }
    else if (movesMade % 10 == 0) {
        moveSequence = (char*)realloc(moveSequence, (movesMade + 10) * sizeof(char));
    }
    if (!moveSequence) {
        printf("Failed to allocate 10 bytes! The program will now exit.\n");
        log_error("Failed to allocate memory for move sequence.");
        exit(1);
    }

    moveSequence[movesMade] = move;
    movesMade++;
}

void handleInteractions() {
    if (metadata HERE == -2)
        return; // Error state, do nothing
    if (map HERE == CHAR_GOAL) {
        victory = 1;
        log_info("Goal was reached.");
    }
    else if ((map HERE == CHAR_KEY) && (metadata HERE != -1)) {
        int id = metadata HERE;
        log_info("Key %d was picked up.", id);
        int doorsOpened = 0;
        for (int r = 0; r < roomCount; r++) {
            for (int i = 0; i < roomWidth; i++) {
                for (int j = 0; j < roomWidth; j++) {
                    if ((map[r][i][j] == CHAR_DOOR) && (metadata[r][i][j] == id)) {
                        metadata[r][i][j] = -1; // Open door
                        log_info("Door %d was unlocked.", id);
                        doorsOpened++;
                    }
                }
            }
        }
        if (doorsOpened == 0) {
            log_warn("No doors were opened with key %d.", id);
        }
        metadata HERE = -1; // Mark key as collected
    }
    else if ((map HERE == CHAR_PASSAGE)) {
        int id = metadata HERE;
        int found = 0; // Find other passage with same ID
        for (int r = 0; r < roomCount; r++) {
            for (int i = 0; i < roomWidth; i++) {
                for (int j = 0; j < roomWidth; j++) {
                    if ((map[r][i][j] == CHAR_PASSAGE) && (metadata[r][i][j] == id) && !(r == playerR && i == playerY && j == playerX)) {
                        playerR = r;
                        playerY = i;
                        playerX = j;
                        found = 1;
                        log_info("Passage %d used to move to room %d at %d,%d.", id, r, j, i);
                        break;
                    }
                }
                if (found) break;
            }
            if (found) break;
        }
        if (!found) {
            metadata HERE = -2; // Mark as error
            log_error("Passage %d is not paired.", id);
        }
    }
}

int movePlayer(char input) {
    int valid = 0;
    switch (input) {
        case 'w':
            if ((map UP == CHAR_WALL) || (map UP == CHAR_DOOR && metadata UP != -1))
                break;    
            playerY--;
            valid = 1;
        break;
        case 's':
            if ((map DOWN == CHAR_WALL) || (map DOWN == CHAR_DOOR && metadata DOWN != -1))
                break;
            playerY++;
            valid = 1;
        break;
        case 'a':
            if ((map LEFT == CHAR_WALL) || (map LEFT == CHAR_DOOR && metadata LEFT != -1))
                break;
            playerX--;
            valid = 1;
        break;
        case 'd':
            if ((map RIGHT == CHAR_WALL) || (map RIGHT == CHAR_DOOR && metadata RIGHT != -1))
                break;
            playerX++;
            valid = 1;
        break;
    }
    if (valid) {
        addMoveToSequence(input);
    }
    return !valid;
}

void handleOutput() {
    // Move cursor to top-left and hide cursor while redrawing to avoid full-screen flash
    HOME_CURSOR();
    HIDE_CURSOR();

    // Print game info
    if (!victory){
        printf("Moves made: %d     Position: %2d, %2d, %2d   \n\n", movesMade, playerX, playerY, playerR);
    } else {
        printf("Moves made: %d\n\n", movesMade);// Position will stay due to lack of redraw
    }

    // Print map
    for (int i = 0; i < roomWidth; i++) {
        for (int j = 0; j < roomWidth; j++) {
            if (i == playerY && j == playerX) {
                printf(TILE_PLAYER);
            } else {
                if (metadata[playerR][i][j] != -2) {// Not error
                    switch (map[playerR][i][j]) {
                        case CHAR_VOID:
                            printf(TILE_VOID);
                        break;
                        case CHAR_WALL:
                            printf(TILE_WALL);
                        break;
                        case CHAR_DOOR:
                            if (metadata[playerR][i][j] != -1)// Not open
                                printf(TILE_DOOR);
                            else
                                printf(TILE_DOOR_RESIDUE);
                        break;
                        case CHAR_KEY:
                            if (metadata[playerR][i][j] != -1)// Not collected
                                printf(TILE_KEY);
                            else
                                printf(TILE_KEY_RESIDUE);
                        break;
                        case CHAR_GOAL:
                            printf(TILE_GOAL);
                        break;
                        case CHAR_PASSAGE:
                            printf(TILE_PASSAGE);
                        break;
                        case CHAR_START:
                            printf(TILE_START_RESIDUE);
                        break;
                        default:
                            printf(TILE_SYMBOL, map[playerR][i][j]);
                        break;
                    }
                } else {
                    printf(TILE_ERROR);
                }
            }
        }
        printf("\n");
    }
    SHOW_CURSOR();
}

void handleInput() {
    // Loop until valid input
    int awaitingInput = 1;
    while (awaitingInput) {
        char input = getch_portable();
        if (input == 'q') {
            CLEAR_SCREEN();
            atMenuGUI = 1; // Q throws into menu
            return;

        } else {
            awaitingInput = movePlayer(input);
        }
    }
}

void loadMoves(char* saveFile) {
    log_info("User opted to load saved game.");
    loading = 1;
    int loadedMoves = 0;
    char *loadedSequence = NULL;
    if (loadData(saveFile, &loadedMoves, &loadedSequence)) {
        for (int i = 0; i < loadedMoves; i++) {
            int invalid = movePlayer(loadedSequence[i]);
            if (invalid) {
                log_warn("Save file contains invalid moves! It might be old or corrupted. Key: %c", loadedSequence[i]);
            }
            handleInteractions();
            printf("Replaying move %d/%d\r", i + 1, loadedMoves);
            fflush(stdout);
        }
        free(loadedSequence);
    }
    else {
        log_error("Failed to load game data from file '%s'.", saveFile);
        exit(1);
    }
    log_info("Success; loaded %d moves", loadedMoves);
    loading = 0;
}

int awaitInputGUI(int clearOnPageChange) {
    // Loop until valid input
    char input;
retry:
    flushInput();
    input = getch_portable();
    int esc = 0;
    switch (input) {
        case 'w': // up
            cursorGUI--;
        break;
        case 's': // down
            cursorGUI++;
        break;
        case ' ':
        case '\n':
        case '\r':
        case 'y':
            submitGUI = 1;
            if (clearOnPageChange) CLEAR_SCREEN();
        break;
        case 'q':
        case 27: // "esc" key
            esc = 1;
            if (clearOnPageChange) CLEAR_SCREEN();
        break;
        default:
            if (input >= '0' && input <= '9')
                cursorGUI = input - '0';
            else
                goto retry;
        break;
    }
    cursorGUI = (cursorGUI + choicesGUI - 1) % choicesGUI + 1;
    return esc;
}

void renderGUI(int padding, int choices, char* title, char **options) {
    HOME_CURSOR();
    HIDE_CURSOR();
    choicesGUI = choices;
    // Calculate width of box based on title length
    int titleLength = (int)strlen(title) + 4 * (padding + 1);

    // Title (top border)
    for (int i = 0; i < padding; i++)
        printf(ANSI_COL("##", "37;100"));
    printf(ANSI_COL("  %s  ", "34;100"), title);
    for (int i = 0; i < padding; i++)
        printf(ANSI_COL("##", "37;100"));
    printf("\n");

    // Free space before options
    printf(ANSI_COL("##", "37;100"));
    printf("\033[%d;%dH", 2, titleLength - 1);
    printf(ANSI_COL("##\n", "37;100"));
    
    // Options
    for (int i = 0; i < choices; i++) {
        int hovered = (i + 1 == cursorGUI) && (choices > 1);
        // Left Border piece
        printf(ANSI_COL("##", "37;100"));

        // Add '>' at the start if hovered on
        if (hovered)
            printf(ANSI_COL(" > ", "34"));
        else
            printf(ANSI_COL("   ", "94"));

        // Numerate only if more than 1 option present (color if hovered)
        if (choices > 1) {
            if (hovered)
                printf(ANSI_COL("%d. ", "34"), i + 1);
            else
                printf(ANSI_COL("%d. ", "94"), i + 1);
        } 
        
        // The Option string (color if hovered)
        if (hovered)
            printf(ANSI_COL("%s", "34"), options[i]);
        else
            printf(ANSI_COL("%s", "94"), options[i]);

        // Add '>' at the start if hovered on
        if (hovered)
            printf(ANSI_COL(" < ", "34"));
        else
            printf(ANSI_COL("   ", "94"));
        
        // Right Border piece
        printf("\033[%d;%dH", i + 3, titleLength - 1);
        printf(ANSI_COL("##\n", "37;100"));
    }

    // Free space after options
    printf(ANSI_COL("##", "37;100"));
    printf("\033[%d;%dH", choices + 3, titleLength - 1);
    printf(ANSI_COL("##\n", "37;100"));

    // Bottom Border
    for (int i = 1; i < titleLength; i += 2)
        printf(ANSI_COL("##", "37;100"));

    // This writes either 1 or 2 chars
    printf("\033[%d;%dH", choices + 4, titleLength - 1);
    printf(ANSI_COL("##", "37;100"));

    // Info about usage
    printf(ANSI_COL("\nNavigate with W/S or numbers", "90"));
    printf(ANSI_COL("\nGo back with Q or Esc", "90"));
    printf(ANSI_COL("\nSelect with Space or Return", "90"));
    printf("\n"); // Move the blinking away from view

    SHOW_CURSOR();
}

void handleGUI() { // This is sort of sphaghetti by definition, because it contains all GUI branches
    CLEAR_SCREEN();
    cursorGUI = 1; // Default on "Back to game"/"Continue"
    int doneWithGUI = 0;
    while(!doneWithGUI) {
        if (isGameLoaded) {
            renderGUI(7, 3, "PAUSED", (char*[]){"Back to game", "Restart", "Quit"});
            if (awaitInputGUI(1)) break;
            if (submitGUI) {
                submitGUI = 0;
                switch (cursorGUI) {
                    case 1:// back to game
                        doneWithGUI = 1;
                    break;
                    case 2:;// restart
                        char level[256];
                        strcpy(level, loadedLevelName);
                        log_info("Restarting level %s", level);
                        unloadGame();
                        loadGame(level);
                        doneWithGUI = 1;
                    break;
                    case 3:// quit
                        cursorGUI = 3; // Default on "Cancel"
                        int doneWithQuit = 0;
                        while (!doneWithQuit) {
                            renderGUI(6, 3, "QUIT", (char*[]){"Save and Quit", "Quit", "Cancel"});
                            if (awaitInputGUI(1)) break;
                            if (submitGUI) {
                                submitGUI = 0;
                                switch (cursorGUI) {
                                    case 1:;// save quit
                                        char savePath[256];
                                        char* playerName = getStringInput("Players name: ");
                                        sprintf(savePath, GAMES_FOLDER"/%s/"ONGOING_FOLDER"/%s.bin", loadedLevelName, playerName);
                                        free(playerName);
                                        log_info("User opted to save the game.");
                                        log_info(savePath);
                                        if (saveData(savePath, movesMade, moveSequence)) {
                                            printf("\nGame saved successfully!\n");
                                            log_info("Success; saved %d moves", movesMade);
                                        } else {
                                            printf("\nFailed to save game.\n");
                                            log_error("Failed to save game data to file.");
                                        }
                                        log_info("Player quit and save the game through pause menu.");
                                        unloadGame();
                                        fetchLocalData();
                                        doneWithQuit = 1;
                                        doneWithGUI = 1;
                                    break;
                                    case 2:// quit no save
                                        log_info("Player quit the game through pause menu (no save).");
                                        unloadGame();
                                        doneWithQuit = 1;
                                        doneWithGUI = 1;
                                    break;
                                    case 3:// cancel
                                        doneWithQuit = 1;
                                    break;
                                }
                            }
                        }
                    break;
                }
            }

        } else {
            renderGUI(7, 4, "MAIN MENU", (char*[]){"Continue", "New Game", "Leaderboard", "Quit"});
            if (awaitInputGUI(1)) break;
            if (submitGUI) {
                submitGUI = 0;
                switch (cursorGUI) {
                    case 1:// continue
                        if (levelCount > 0) {
                            cursorGUI = 1;
                            int doneWithLevelSaveSelect = 0;
                            while (!doneWithLevelSaveSelect) {
                                renderGUI(8, levelCount, "LEVEL SELECT", levelNames);
                                if (awaitInputGUI(1)) break;
                                if (submitGUI) {
                                    submitGUI = 0;
                                    int levelIndex = cursorGUI - 1;
                                    if (ongoingGameCounts[levelIndex]) {
                                        cursorGUI = 1;
                                        int doneWithSaveSelect = 0;
                                        while (!doneWithSaveSelect) {
                                            renderGUI(8, ongoingGameCounts[levelIndex], "SAVE SELECT", ongoingPlayerNames[levelIndex]);
                                            if (awaitInputGUI(1)) break;
                                            if (submitGUI) {
                                                submitGUI = 0;
                                                int saveIndex = cursorGUI - 1;
                                                loadGame(levelNames[levelIndex]);
                                                char fullSavePath[256];
                                                sprintf(fullSavePath, GAMES_FOLDER"/%s/"ONGOING_FOLDER"/%s.bin", levelNames[levelIndex], ongoingPlayerNames[levelIndex][saveIndex]);
                                                loadMoves(fullSavePath);
                                                doneWithSaveSelect = 1;
                                                doneWithLevelSaveSelect = 1;
                                                doneWithGUI = 1;
                                            }
                                        }
                                    } else {
                                        renderGUI(9, 1, "NOTE", (char*[]){"No ongoing games for this level!"});
                                        getch_portable();
                                        CLEAR_SCREEN();
                                    }
                                }
                            }
                        } else {
                            renderGUI(9, 1, "NOTE", (char*[]){"No levels found!"});
                            getch_portable();
                            CLEAR_SCREEN();
                        }
                    break;
                    case 2:// new game
                        if (levelCount > 0) {
                            cursorGUI = 1;
                            int doneWithLevelNewSelect = 0;
                            while (!doneWithLevelNewSelect) {
                                renderGUI(8, levelCount, "LEVEL SELECT", levelNames);
                                if (awaitInputGUI(1)) break;
                                if (submitGUI) {
                                    submitGUI = 0;
                                    int levelIndex = cursorGUI - 1;
                                    loadGame(levelNames[levelIndex]);
                                    doneWithLevelNewSelect = 1;
                                    doneWithGUI = 1;
                                }
                            }
                        } else {
                            renderGUI(9, 1, "NOTE", (char*[]){"No levels found!"});
                            getch_portable();
                            CLEAR_SCREEN();
                        }
                    break;
                    case 3:// leaderboard
                        if (levelCount > 0) {
                            cursorGUI = 1;
                            int doneWithLeaderboard = 0;
                            while (!doneWithLeaderboard) {
                                renderGUI(8, levelCount, "LEVEL SELECT", levelNames);
                                if (awaitInputGUI(1)) {
                                    doneWithLeaderboard = 1;
                                    break;
                                }
                                if (submitGUI) {
                                    submitGUI = 0;
                                    int levelIndex = cursorGUI - 1;
                                    int numFinished = finishedGameCounts[levelIndex];
                                    if (numFinished == 0) {
                                        renderGUI(9, 1, "NOTE", (char*[]){"No finished games for this level!"});
                                        getch_portable();
                                        CLEAR_SCREEN();
                                    } else {
                                        // Sort by moves count (ascending)
                                        int* indices = (int*)malloc(numFinished * sizeof(int));
                                        for (int i = 0; i < numFinished; i++) indices[i] = i;
                                        for (int i = 0; i < numFinished - 1; i++) {
                                            for (int j = 0; j < numFinished - i - 1; j++) {
                                                if (finishedMovesCounts[levelIndex][indices[j]] > finishedMovesCounts[levelIndex][indices[j+1]]) {
                                                    int temp = indices[j];
                                                    indices[j] = indices[j+1];
                                                    indices[j+1] = temp;
                                                }
                                            }
                                        }
                                        // Create leaderboard options
                                        char** leaderboardOptions = (char**)malloc(numFinished * sizeof(char*));
                                        for (int i = 0; i < numFinished; i++) {
                                            int idx = indices[i];
                                            leaderboardOptions[i] = (char*)malloc(256 * sizeof(char));
                                            sprintf(leaderboardOptions[i], "%s: %d moves", finishedPlayerNames[levelIndex][idx], finishedMovesCounts[levelIndex][idx]);
                                        }
                                        // Render leaderboard
                                        cursorGUI = 1;
                                        int doneWithLeaderboard = 0;
                                        while (!doneWithLeaderboard) {
                                            renderGUI(10, numFinished, "LEADERBOARD", leaderboardOptions);
                                            // User can move around the leaderboard, but both submit and back will send back.
                                            if (awaitInputGUI(1)) break;
                                            if (submitGUI) {
                                                submitGUI = 0;
                                                cursorGUI = levelIndex + 1;
                                                doneWithLeaderboard = 1;
                                            }
                                        }
                                        for (int i = 0; i < numFinished; i++) free(leaderboardOptions[i]);
                                        free(leaderboardOptions);
                                        free(indices);
                                    }
                                }
                            }
                        } else {
                            renderGUI(9, 1, "NOTE", (char*[]){"No levels found!"});
                            getch_portable();
                            CLEAR_SCREEN();
                        }
                    break;
                    case 4:// quit
                        log_info("User quit the game");
                        quitting = 1;
                        doneWithGUI = 1;
                    break;
                }
            }
        }
    }
    atMenuGUI = 0;
}

void animateVictory() {
    int goalX = playerX;
    int goalY = playerY;
    int directions[4][2] = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} }; // Up, Right, Down, Left
    int dirIndex = 0;
    int lineLength = 1;
    playerX = playerY = -1; // Move player off map during animation
    handleOutput();
    int tileCount = roomWidth * roomWidth;
    // Spiral around victory tile
    while (lineLength <= roomWidth * 2) {
        for (int dir = 0; dir < 4; dir++) {// For each direction
            for (int step = 0; step < lineLength; step++) { // For each step in that direction
                if (dir == 0 || dir == 2) // Up or Down
                    goalY += directions[dir][1];
                else // Right or Left
                    goalX += directions[dir][0];
                if (goalX < 0 || goalX >= roomWidth || goalY < 0 || goalY >= roomWidth)
                    continue; // Skip out-of-bounds
                usleep(100000 / lineLength + 1); // spiral goes faster as it expands
                map[playerR][goalY][goalX] = CHAR_GOAL;
                handleOutput();
                printf("\nCongratulations! You've escaped the maze in %d moves!\n", movesMade);
            }
            if (dir == 1 || dir == 3) { // After Right or Left, increase line length
                lineLength++;
            }
        }
    }
}

void handleGame() {
    if (isGameLoaded) {
        if (!victory){
            handleOutput();
            printf(ANSI_COL("\nUse WASD to move, Q to quit.\n", "90"));
            handleInput();
            if (atMenuGUI) return; // skip interactions if user went to GUI
            handleInteractions();
        } else {
            animateVictory();
            flushInput(); // Flush any input possibly made during animation
            getch_portable(); // Wait for final input

            // Save finished game to leaderboard
            char* playerName = getStringInput("Enter your name for the leaderboard: ");
            // Sanitize playerName for filename
            if (strlen(playerName) == 0) {
                free(playerName);
                playerName = strdup("Anonymous");
            } else {
                for (char *p = playerName; *p; p++) {
                    if (*p == '/' || *p == '\\' || *p == ':' || *p == '*' || *p == '?' || *p == '"' || *p == '<' || *p == '>' || *p == '|') {
                        *p = '_';
                    }
                }
            }
            char savePath[256];
            sprintf(savePath, GAMES_FOLDER"/%s/"FINISHED_FOLDER"/%s.bin", loadedLevelName, playerName);
            if (saveData(savePath, movesMade, moveSequence)) {
                printf("\nScore saved to leaderboard!\n");
                log_info("Finished game saved for %s", playerName);
            } else {
                printf("\nFailed to save score.\n");
                log_error("Failed to save finished game.");
            }
            free(playerName);
            fetchLocalData();
            unloadGame();
        }
        
    } else {
        // This screen should be unreachable, though exists for future refactoring
        CLEAR_SCREEN();
        printf(ANSI_COL("Make sure to use fullscreen.\n", "90"));
        printf(ASCII_LOGO);
        getch_portable();
        CLEAR_SCREEN();
        atMenuGUI = 1;
    }
}

int main() {
    // Initialize logging
    log_start();
    CLEAR_SCREEN();

    // Check associated files
    fetchLocalData();

    // game loop
    while (!quitting) {
        if (atMenuGUI){
            handleGUI();
        } else {
            handleGame();
        }
    }

    // Free resources
    freeLocalData();

    // Good bye
    CLEAR_SCREEN();
    printf("Good bye!\n");
    getch_portable();

    exit(0);
}