// Since usage of malloc/realloc is a requirement and maze game is deterministic, save system will work based on moves made. It will not save player's position neither know if player has collected the key

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "binio.h"

#define SAVE_FILE "save.bin"

// Macro for ANSI escape codes
#define ANSI_ESC(text, code) "\x1B[" code "m" text "\x1B[0m"

// Clear screen and getch portable implementations
#ifdef _WIN32
#include <conio.h>
#define CLEAR_SCREEN() system("cls")
char getch_portable() {
    return getch();
}
#else
#include <termios.h>
#include <unistd.h>
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
#endif

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

int hasKey = 0;
int victory = 0;

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
        exit(1);
    }

    moveSequence[movesMade] = move;
    movesMade++;
}

void handleInteractions(){
    if (map[playerY][playerX] == 'W') {
        victory = 1;
    }
    else if (map[playerY][playerX] == 'K') {
        hasKey = 1;
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
    printf("Moves made: %d\tKey: %s\n\n", movesMade, hasKey ? ANSI_ESC("o+", "92") : ANSI_ESC("__", "92"));

    // Print map
    for (int i = 0; i < MAP_WIDTH; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            if (i == playerY && j == playerX) {
                printf("()");
            }
            else {
                // Map blocks are 2 characters wide for justification
                switch (map[i][j]) {
                    case ' ': // Void
                        printf("  ");
                        break;
                    case '#': // Wall
                        printf(ANSI_ESC("##", "107"));
                        break;
                    case '@': // Door
                        if (hasKey) {
                            printf("  ");
                        }
                        else{
                            printf(ANSI_ESC("@@", "42"));
                        }
                        break;
                    case 'K': // Key
                        if (!hasKey) {
                            printf(ANSI_ESC("o+", "92;1"));
                        }
                        else {
                            printf("  ");
                        }
                        break;
                    case 'W': // Victory
                        printf(ANSI_ESC("[]", "103;95"));
                        break;
                }
            }
        }
        printf("\n");
    }

    printf("\n\nUse WASD to move, Q to quit.\n");
}

void handleInput() {
    // Loop until valid input
    int awaitingInput = 1;
    while (awaitingInput) {
        char input = getch_portable();
        if (input == 'q') {
            printf("\nExiting.\n\nWould you like to save your progress? Y/N\n");
            // Prompt user to save progress
            char saveInput = 0;
            while (saveInput != 'y' && saveInput != 'n') {
                saveInput = getch_portable();
            }

            // Save game state
            if (saveInput == 'y') {
                if (saveData(SAVE_FILE, movesMade, moveSequence)) {
                    printf("\nGame saved successfully!\n");
                } else {
                    printf("\nFailed to save game.\n");
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
            int loadedMoves = 0;
            char *loadedSequence = NULL;
            if (loadData(SAVE_FILE, &loadedMoves, &loadedSequence)) {
                for (int i = 0; i < loadedMoves; i++) {
                    movePlayer(loadedSequence[i]);
                    handleInteractions();
                    printf("Replaying move %d/%d\r", i + 1, loadedMoves);
                    fflush(stdout);
                }
                free(loadedSequence);
            }
        }
    }

    // Loop game until victory
    while (!victory){
        handleOutput();
        handleInput();
        handleInteractions();
    }

    printf("\nCongratulations! You've escaped the maze in %d moves!\n", movesMade);

    // Remove save file
    deleteData(SAVE_FILE);

    return 0;
}
