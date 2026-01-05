#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "loglib.h"

static void ensureTmpDirectory(void) {
    _mkdir("tests\\tmp");
}

int main(void) {
    ensureTmpDirectory();
    const char *path = "tests/tmp/log_test.txt";
    remove(path);

    log_start_path(path);
    log_info("example info");
    log_warn("example warn");
    log_error("example error");
    
    
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "FAIL: could not open log file '%s'\n", path);
        return 1;
    }

    char buf[8192] = {0};
    size_t n = fread(buf, 1, sizeof(buf)-1, f);
    fclose(f);

    if (n == 0) {
        fprintf(stderr, "FAIL: log file appears empty, path: '%s'\n", path);
        return 2;
    }

    if (!strstr(buf, "INFO") || !strstr(buf, "WARN") || !strstr(buf, "ERROR")) {
        fprintf(stderr, "FAIL: one or more appear missing, path: '%s'\n", path);
        return 3;
    }

    if (strstr(buf, "example info") == NULL ||
        strstr(buf, "example warn") == NULL ||
        strstr(buf, "example error") == NULL) {
        fprintf(stderr, "FAIL: one or more messages missing, path: '%s'\n", path);
        return 4;
    }

    printf("PASS: file appears to contain all logs, path: '%s'\n", path);
    return 0;
}
