// Since usage of malloc/realloc is a requirement and maze game is deterministic, save system will work based on moves made. It will not save player's position neither know if player has collected the key

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macro for ANSI escape codes
#define SPECIAL_TEXT(text, code) "\x1B[" #code "m" text "\x1B[0m"

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

int playerX = 1;
int playerY = 1;

int hasKey = 0;
int victory = 0;

int movesMade = 0;
char *moveSequence = NULL;

void addMoveToSequence(char move);
int findData();
void saveData();
void loadData();
void deleteData();
void handleOutput();
void handleInput();
void handleInteractions();
int movePlayer(char input);

int main() {
    // Intro
    printf("\nWelcome to The Maze!\n");
    getch_portable();

    // Search for bin save file
    if (findData()) {
        char loadInput;
        printf("\nSave file found! Continue? Y/N\n");

        // Prompt user to load save
        while (loadInput != 'y' && loadInput != 'n') {
            loadInput = getch_portable();
        }

        // Load game state
        if (loadInput == 'y') {
            loadData();
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
    deleteData();

    return 0;
}

// Function to resize move sequence array using realloc
void addMoveToSequence(char move) {
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

int findData(){
    FILE *file = fopen("save.bin", "rb");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

void saveData(){
    FILE *file = fopen("save.bin", "wb");
    fwrite(&movesMade, sizeof(int), 1, file);
    fwrite(moveSequence, sizeof(char), movesMade, file);
    fclose(file);
}

void loadData(){
    FILE *file = fopen("save.bin", "rb");
    if (file) {
        int loadedMoves;
        fread(&loadedMoves, sizeof(int), 1, file);
        char *loadedSequence = (char*)malloc(loadedMoves * sizeof(char));
        fread(loadedSequence, sizeof(char), loadedMoves, file);
        fclose(file);
        for (int i = 0; i < loadedMoves; i++) {
            movePlayer(loadedSequence[i]);
            handleInteractions();
            printf("Replaying move %d/%d\r", i + 1, loadedMoves);
            fflush(stdout);
        }
        free(loadedSequence);
    }
}

void deleteData(){
    remove("save.bin");
}

void handleOutput(){
    // Clear screen before printing
    CLEAR_SCREEN();

    // Print game info
    printf("Moves made: %d\tKey: %s\n\n", movesMade, hasKey ? "o+" : "__");

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
                        printf("##");
                        break;
                    case '@': // Door
                        if (hasKey) {
                            printf("  ");
                        }
                        else{
                            printf(SPECIAL_TEXT("@@", 2));
                        }
                        break;
                    case 'K': // Key
                        if (!hasKey) {
                            printf("o+");
                        }
                        else {
                            printf("__");
                        }
                        break;
                    case 'W': // Victory
                        printf("[]");
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
            char saveInput;
            while (saveInput != 'y' && saveInput != 'n') {
                saveInput = getch_portable();
            }

            // Save game state
            if (saveInput == 'y') {
                FILE *saveFile = fopen("save.bin", "wb");
                if (saveFile) {
                    saveData();
                    printf("\nGame saved successfully!\n");
                }
            }
            exit(0);
        }
        else {
            awaitingInput = movePlayer(input);
        }
    }
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
    switch (input) {
        case 'w': // UP
            if (map[playerY - 1][playerX] != '#' && (map[playerY - 1][playerX] != '@' || hasKey)) {
                playerY--;
            }
            break;
        case 's': // DOWN
            if (map[playerY + 1][playerX] != '#' && (map[playerY + 1][playerX] != '@' || hasKey)) {
                playerY++;
            }
            break;
        case 'a': // LEFT
            if (map[playerY][playerX - 1] != '#' && (map[playerY][playerX - 1] != '@' || hasKey)) {
                playerX--;
            }
            break;
        case 'd': // RIGHT
            if (map[playerY][playerX + 1] != '#' && (map[playerY][playerX + 1] != '@' || hasKey)) {
                playerX++; 
            }
            break;
        default:
            return 1; // Invalid input
    }
    addMoveToSequence(input);
    return 0; // Valid move

}
