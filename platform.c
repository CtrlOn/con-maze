#include "platform.h"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>

int usleep(unsigned int usec) {
    Sleep((usec + 999) / 1000);
    return 0;
}

char getch_portable(void) {
    return getch();
}

void flushInput(void) {
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
}

void platform_clear_screen(void) {
    system("cls");
}

void platform_home_cursor(void) {
    printf("\033[H");
}

#else // POSIX

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

char getch_portable(void) {
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

void flushInput(void) {
    tcflush(STDIN_FILENO, TCIFLUSH);
}

void platform_clear_screen(void) {
    printf("\033[2J\033[H");
}

void platform_home_cursor(void) {
    printf("\033[H");
}

#endif
