// Since usage of malloc/realloc is a requirement and maze game is deterministic, save system will work based on moves made. It will not save player's position neither know if player has collected the key

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "binio.h"
#include "loglib.h"

#define SAVE_FILE "save.bin"
#define LOG_FILE "runtime.log"

// Macro for ANSI escape codes
#define ANSI_ESC(text, code) "\x1B[" code "m" text "\x1B[0m"

// Clear screen and getch portable implementations
#ifdef _WIN32
#include <conio.h>
#include <windows.h>

// usleep replacement for Windows
static inline int usleep(unsigned int usec) {
    Sleep((usec + 999) / 1000);
    return 0;
}

#define CLEAR_SCREEN() system("cls")
char getch_portable() {
    return getch();
}

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
#endif // _WIN32

// Tiles 
// Get color codes from here https://i.sstatic.net/9UVnC.png
#define TILE_PLAYER     ANSI_ESC("()", "94;44")
#define TILE_VOID       ANSI_ESC("  ", "90;40")
#define TILE_WALL       ANSI_ESC("##", "37;100")
#define TILE_DOOR       ANSI_ESC("##", "92;42")
#define TILE_KEY        ANSI_ESC("o+", "92;40")
#define TILE_GOAL       ANSI_ESC("[]", "93;43")
#define TILE_PASSAGE    ANSI_ESC("[]", "92;42")
#define TILE_SYMBOL(s)  ANSI_ESC(s " ", "90;40")

// Build maze here
#define MAP_WIDTH 16

char map[MAP_WIDTH][MAP_WIDTH] = {
    "################",
    "#        # #   #",
    "#   #### # # W #",
    "#   # #    @   #",
    "##### #### #####",
    "#      #   # # #",
    "# #  # # #     #",
    "# ## #   # ### #",
    "#  #@##  #   ###",
    "# ##  ###### # #",
    "# #   #    #   #",
    "# # # @  # ### #",
    "#   ###  #   # #",
    "# # #    # K # #",
    "# #    # #   # #",
    "################"
};

// Starting position
int playerX = 1;
int playerY = 1;

// Global flags
int hasKey = 0;
int victory = 0;
int loading = 0;

int movesMade = 0;
char *moveSequence = NULL;

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
    if (map[playerY][playerX] == 'W') {
        victory = 1;
        log_info("Goal was reached!");
    }
    else if (map[playerY][playerX] == 'K') {
        hasKey = 1;
        log_info("Key was picked up!");
    }
}

int movePlayer(char input) {
    int valid = 0;
    switch (input) {
        case 'w': // UP
            if (map[playerY - 1][playerX] != '#' && (map[playerY - 1][playerX] != '@' || hasKey)) {
                playerY--;
                valid = 1;
            }
            break;
        case 's': // DOWN
            if (map[playerY + 1][playerX] != '#' && (map[playerY + 1][playerX] != '@' || hasKey)) {
                playerY++;
                valid = 1;
            }
            break;
        case 'a': // LEFT
            if (map[playerY][playerX - 1] != '#' && (map[playerY][playerX - 1] != '@' || hasKey)) {
                playerX--;
                valid = 1;
            }
            break;
        case 'd': // RIGHT
            if (map[playerY][playerX + 1] != '#' && (map[playerY][playerX + 1] != '@' || hasKey)) {
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
    // Clear screen before printing
    CLEAR_SCREEN();

    // Print game info
    printf("Moves made: %d\tKey: %s\n\n", movesMade, hasKey ? TILE_KEY : ANSI_ESC("__", "92"));

    // Print map
    for (int i = 0; i < MAP_WIDTH; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            if (i == playerY && j == playerX) {
                printf(TILE_PLAYER);
            }
            else {
                // Map blocks are 2 characters wide for justification
                switch (map[i][j]) {
                    case ' ': // Void
                        printf(TILE_VOID);
                        break;
                    case '#': // Wall
                        printf(TILE_WALL);
                        break;
                    case '@': // Door
                        if (hasKey) {
                            printf(TILE_VOID);
                        }
                        else{
                            printf(TILE_DOOR);
                        }
                        break;
                    case 'K': // Key
                        if (!hasKey) {
                            printf(TILE_KEY);
                        }
                        else {
                            printf(TILE_VOID);
                        }
                        break;
                    case 'W': // Goal
                        printf(TILE_GOAL);
                        break;
                }
            }
        }
        printf("\n");
    }
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
    printf("\nWelcome to The Maze!\n");
    getch_portable();

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
                    int valid = movePlayer(loadedSequence[i]);
                    if (!valid) {
                        log_warn("Save file contains invalid moves! It might be old or corrupted. Key: %c", loadedSequence[i]);
                    }
                    handleInteractions();
                    printf("Replaying move %d/%d\r", i + 1, loadedMoves);
                    fflush(stdout);
                }
                free(loadedSequence);
            }
            log_info("Success; loaded %d moves", loadedMoves);
            loading = 0;
        }
    }

    // Loop game until victory
    while (!victory){
        handleOutput();
        printf("\n\nUse WASD to move, Q to quit.\n");
        handleInput();
        handleInteractions();
    }

    // Victory animation
    playerX = playerY = -1; // Move player off map during animation
    for (int i = 0; i < MAP_WIDTH; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            map[i][j] = '#';
        }
        handleOutput();
        printf("\n\nCongratulations! You've escaped the maze in %d moves!\n", movesMade); // Victory message replaces movement instructions
        usleep(100000); // 100 ms delay
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