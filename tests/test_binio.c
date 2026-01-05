#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "binio.h"

static void ensureTmpDirectory(void) {
    _mkdir("tests\\tmp");
}

int main(void) {
    ensureTmpDirectory();

    const char *path = "tests/tmp/binio_test.bin";
    const char sample[] = "QWERTYUIOPASDFGHJKLZXCVBNM!@#$^&*()-=[];'./,<>?:\"{}|`~";
    const int count = (int)strlen(sample);

    remove(path);
    if (!saveData(path, count, sample)) {
        printf("FAIL: saveData failed\n");
        return 1;
    }

    int mismatchCount = 0;
    char *out = NULL;
    if (!loadData(path, &mismatchCount, &out)) {
        printf("FAIL: loadData failed\n");
        return 1;
    }

    if (mismatchCount != count || memcmp(out, sample, count) != 0) {
        printf("FAIL: data mismatch (%d mismatches)\n", mismatchCount);
        free(out);
        return 2;
    }

    free(out);
    remove(path);
    printf("PASS: save/load consistent\n");
    return 0;
}
