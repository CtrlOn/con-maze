// Since usage of malloc/realloc is a requirement and maze game is deterministic, save system will work based on moves made. It will not save player's position neither know if player has collected the key

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "binio.h"
#include "loglib.h"

#define SAVE_FILE "save.bin"
#define LOG_FILE "runtime.log"
#define LEVEL_FILE "level.dat"

// Macro for ANSI escape codes
#define ANSI_ESC(text, code) "\x1B[" code "m" text "\x1B[0m"

#define HIDE_CURSOR() printf("\033[?25l")
#define SHOW_CURSOR() printf("\033[?25h")

// Clear screen and getch portable implementations
#ifdef _WIN32
#include <conio.h>
#include <windows.h>

// Some toolchains (older MinGW, etc.) may not expose
//  ENABLE_VIRTUAL_TERMINAL_PROCESSING. Provide a fallback
//  value so code compiles. The value 0x0004 matches the
//  value used in modern Windows SDKs.
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// enable VT processing so ANSI escapes work on Windows consoles (best-effort)
static void enable_vt_mode(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
}

// usleep replacement for Windows
static inline int usleep(unsigned int usec) {
    Sleep((usec + 999) / 1000);
    return 0;
}

#define CLEAR_SCREEN() system("cls")
char getch_portable() {
    return getch();
}

// move cursor to top-left using ANSI sequence (requires VT mode on Windows)
#define HOME_CURSOR() printf("\033[H")

// Drain pending console input using the Windows API.
static void flushInput(void) {
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
}

#else // POSIX
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#define CLEAR_SCREEN() printf("\033[2J\033[H")
// move cursor to top-left (no clear)
#define HOME_CURSOR() printf("\033[H")
char getch_portable() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// Drain pending stdin bytes using tcflush (portable POSIX).
static void flushInput(void) {
    tcflush(STDIN_FILENO, TCIFLUSH);
}
#endif

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
#define TILE_PLAYER         ANSI_ESC("<>", "91;41")
#define TILE_START_RESIDUE  ANSI_ESC("[]", "90;40")
#define TILE_VOID           ANSI_ESC("  ", "90;40")
#define TILE_WALL           ANSI_ESC("##", "37;100")
#define TILE_DOOR           ANSI_ESC("##", "96;46")
#define TILE_DOOR_RESIDUE   ANSI_ESC("##", "90;40") // Shows up when door is open (id = -1)
#define TILE_KEY            ANSI_ESC("o+", "96;40")
#define TILE_KEY_RESIDUE    ANSI_ESC("__", "90;40") // Shows up when key is collected (id = -1)
#define TILE_GOAL           ANSI_ESC("[]", "93;43")
#define TILE_PASSAGE        ANSI_ESC("[]", "93;42")
#define TILE_ERROR          ANSI_ESC("??", "30;105") // Shows up when flagged (id = -2)
#define TILE_SYMBOL         ANSI_ESC("%c ", "90;40") // For text symbols

int roomWidth;
int roomCount;
char*** map;
int*** metadata;
int playerX;
int playerY;
int playerR;

int victory = 0;
int loading = 0;
int movesMade = 0;
char *moveSequence = NULL;

void loadLevel(){
    log_info("Loading level from %s", LEVEL_FILE);
    FILE *f = fopen(LEVEL_FILE, "r");
    if (!f) {
        log_error("Failed to open level file '%s'.", LEVEL_FILE);
        exit(1);
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
        exit(1);
    }
    if (width > 32) {
        log_warn("Rooms of high width might not fit in console window.");
    }
    roomWidth = width;

    // Find START marker
    int in_rooms = 0;
    char **tileLines = NULL;
    size_t tileCount = 0, tileCap = 0;
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
            if (strcmp(p, "START") == 0) {
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
        log_error("No room data found between START and END.");
        exit(1);
    }

    if (tileCount % roomWidth != 0) {
        log_warn("Number of tile lines (%zu) is not a multiple of WIDTH (%d). Truncating excess lines.", tileCount, roomWidth);
    }
    roomCount = (int)(tileCount / roomWidth);
    if (roomCount <= 0) {
        log_error("No valid rooms found in level data.");
        exit(1);
    }

    // Allocate map and metadata
    map = (char***)malloc(roomCount * sizeof(char**));
    metadata = (int***)malloc(roomCount * sizeof(int**));
    for (int r = 0; r < roomCount; ++r) {
        map[r] = (char**)malloc(roomWidth * sizeof(char*));
        metadata[r] = (int**)malloc(roomWidth * sizeof(int*));
        for (int i = 0; i < roomWidth; ++i) {
            map[r][i] = (char*)malloc(roomWidth * sizeof(char));
            metadata[r][i] = (int*)malloc(roomWidth * sizeof(int));
            for (int j = 0; j < roomWidth; ++j) metadata[r][i][j] = 0;
        }
    }

    // Parse rooms
    int foundStart = 0;
    int foundGoal = 0;
    for (int r = 0; r < roomCount; ++r) {
        // Collect metadata tokens for this room in order
        int *metaList = NULL;
        size_t metaCount = 0, metaCap = 0;

        for (int i = 0; i < roomWidth; ++i) {
            size_t idx = (size_t)r * roomWidth + i;
            if (idx >= tileCount) break;
            char *lineptr = tileLines[idx];
            size_t linelen = strlen(lineptr);
            // first roomWidth characters are tile chars (pad with walls if short)
            int lowLength = 0;
            for (int j = 0; j < roomWidth; ++j) {
                if ((size_t)j < linelen) {
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
            if (linelen > (size_t)roomWidth) {
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
        size_t metaIndex = 0;
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
        exit(1);
    }
    if (foundStart > 1) {
        log_error("Multiple start tiles '@' found in level data (%d).", foundStart);
        exit(1);
    }
    if (!foundGoal) {
        log_warn("No goal tile '$' found in level data (any room).");
    }

    // Cleanup tileLines
    for (size_t i = 0; i < tileCount; ++i) free(tileLines[i]);
    free(tileLines);

    log_info("Loaded level: WIDTH=%d, ROOM_COUNT=%d", roomWidth, roomCount);
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

void handleInteractions(){
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
            if (!(map UP == CHAR_WALL) && !(map UP == CHAR_DOOR && metadata UP != -1)) {
                playerY--;
                valid = 1;
            }
            break;
        case 's':
            if (!(map DOWN == CHAR_WALL) && !(map DOWN == CHAR_DOOR && metadata DOWN != -1)) {
                playerY++;
                valid = 1;
            }
            break;
        case 'a':
            if (!(map LEFT == CHAR_WALL) && !(map LEFT == CHAR_DOOR && metadata LEFT != -1)) {
                playerX--;
                valid = 1;
            }
            break;
        case 'd':
            if (!(map RIGHT == CHAR_WALL) && !(map RIGHT == CHAR_DOOR && metadata RIGHT != -1)) {
                playerX++; 
                valid = 1;
            }
            break;
    }
    if (valid) {
        addMoveToSequence(input);
    }
    return !valid;
}

void handleOutput(){
    // Move cursor to top-left and hide cursor while redrawing to avoid full-screen flash
    HOME_CURSOR();
    HIDE_CURSOR();

    // Print game info
    printf("Moves made: %4d\t  Position: %2d, %2d, %2d   \n\n", movesMade, playerX, playerY, playerR);

    // Print map
    for (int i = 0; i < roomWidth; i++) {
        for (int j = 0; j < roomWidth; j++) {
            if (i == playerY && j == playerX) {
                printf(TILE_PLAYER);
            }
            else {
                if (map[playerR][i][j] != -2) {// Not error
                    switch (map[playerR][i][j]) {
                        case CHAR_VOID:
                            printf(TILE_VOID);
                            break;
                        case CHAR_WALL:
                            printf(TILE_WALL);
                            break;
                        case CHAR_DOOR:
                            if (metadata[playerR][i][j] != -1) {// Not open
                                printf(TILE_DOOR);
                            }
                            else{
                                printf(TILE_DOOR_RESIDUE);
                            }
                            break;
                        case CHAR_KEY:
                            if (metadata[playerR][i][j] != -1) {// Not collected
                                printf(TILE_KEY);
                            }
                            else {
                                printf(TILE_KEY_RESIDUE);
                            }
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
                }
                else {
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
            printf("\nExiting.\n\nWould you like to save your progress? Y/N\n");
            log_info("User opted to quit the game.");
            // Prompt user to save progress
            char saveInput = 0;
            while (saveInput != 'y' && saveInput != 'n') {
                saveInput = getch_portable();
            }

            // Save game state
            if (saveInput == 'y') {
                log_info("User opted to save the game.");
                if (saveData(SAVE_FILE, movesMade, moveSequence)) {
                    printf("\nGame saved successfully!\n");
                    log_info("Success; saved %d moves", movesMade);
                } else {
                    printf("\nFailed to save game.\n");
                    log_error("Failed to save game data to file.");
                }
            }
            exit(0);
        }
        else {
            awaitingInput = movePlayer(input);
        }
    }
}

int main() {
    // Initialize logging
    log_start(LOG_FILE);

    // Intro
    printf("Welcome to The Maze!\n");
    getch_portable();
    
    // Load level
    loadLevel();

    // Search for bin save file
    if (findData(SAVE_FILE)) {
        char loadInput = 0;
        printf("\nSave file found! Continue? Y/N\n");

        // Prompt user to load save
        while (loadInput != 'y' && loadInput != 'n') {
            loadInput = getch_portable();
        }

        // Load game state
        if (loadInput == 'y') {
            log_info("User opted to load saved game.");
            loading = 1;
            int loadedMoves = 0;
            char *loadedSequence = NULL;
            if (loadData(SAVE_FILE, &loadedMoves, &loadedSequence)) {
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
                log_error("Failed to load game data from file '%s'.", SAVE_FILE);
                exit(1);
            }
            log_info("Success; loaded %d moves", loadedMoves);
            loading = 0;
        }
    }

    CLEAR_SCREEN();

    // Loop game until victory
    while (!victory){
        handleOutput();
        printf("\n\nUse WASD to move, Q to quit.\n");
        handleInput();
        handleInteractions();
    }

    // Victory animation
    playerX = playerY = -1; // Move player off map during animation
    handleOutput();
    usleep(500000); // 0.5 s delay
    for (int i = 0; i < roomWidth; i++) {
        for (int j = 0; j < roomWidth; j++) {
            map[playerR][i][j] = CHAR_WALL;
            handleOutput();
            printf("\n\nCongratulations! You've escaped the maze in %d moves!\n", movesMade); // Victory message replaces movement instructions
            usleep(20000); // 20 ms delay
        }
    }

    // Wait for final input
    // Drain any accidental keypresses produced during the animation,
    // then block once waiting for a fresh intentional keypress.
    flushInput();
    (void)getch_portable();

    // Remove save file
    deleteData(SAVE_FILE);

    exit(0);
}