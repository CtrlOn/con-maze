#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include "savesdir.h"

#define PADDING "                              "

void flushInput(void) {
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
}

void promptMonitor(void) {
    printf("For this test, external memory monitoring is required.\n");
    printf("Press 'r' to open Resource Monitor\nPress 't' to open Task Manager\nPress 'n' to skip.\n");

    flushInput();
    char c = 0;
    do {
        printf("\r");
        c = getch();
    } while (c != 'r' && c != 't' && c != 'n');

    if (c == 'r')
        system("resmon");
    else if (c == 't')
        system("taskmgr");
    printf("\n");
}

int main(void) {
    promptMonitor();
    printf("Press 'f' to reload local data once\n");
    printf("Press 'g' to reload local data 100 times\n");
    printf("Press 't' to reload local data 10000 times\n");
    printf("Press 'q' to finish test\n");
    
    char c;
    do {
        printf("\r");
        c = getch();
        if (c == 'f') {
            printf("\rWorking...%s", PADDING);
            fetchLocalData();
            freeLocalData();
            printf("\rCompleted.");
        } else if (c == 'g') {
            for (int i = 0; i < 100; i++) {
                printf("\rWorking... (Iteration %d/100) ", i + 1);
                fetchLocalData();
                freeLocalData();
            }
            printf("\rCompleted.%s", PADDING);
        } else if (c == 't') {
            for (int i = 0; i < 10000; i++) {
                printf("\rWorking... (Iteration %d/10000) ", i + 1);
                fetchLocalData();
                freeLocalData();
            }
            printf("\rCompleted.%s", PADDING);
        }
        flushInput();
    } while (c != 'q');
    printf("\rTest finished.\n");

    return 0;
}
