
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "testsearch.h"

#include "testload.h"
#include "util.h"

#define INITIAL_TEST_LIST_CAPACITY 32

void initTestList(TestList* tests) {
    tests->tests = NULL;
    tests->count = 0;
    tests->capacity = 0;
}

void deinitTestList(TestList* tests) {
    for (int i = 0; i < tests->count; i++) {
        deinitTest(&tests->tests[i]);
    }
    free(tests->tests);
}

static void makeSpaceInTestList(TestList* tests) {
    if (tests->count == tests->capacity) {
        if (tests->capacity == 0) {
            tests->capacity = INITIAL_TEST_LIST_CAPACITY;
        } else {
            tests->capacity *= 2;
        }
        tests->tests = (TestCase*)realloc(tests->tests, sizeof(TestCase) * tests->capacity);
    }
}

static void searchForDefaultSettings(const char* dir, TestCaseConfig* config) {
    int length = strlen(dir);
    char path[length + 20];
    memcpy(path, dir, length);
    memcpy(path + length, "/tested.default", 16);
    FILE* file = fopen(path, "r");
    if (file != NULL) {
        fprintf(stderr, "config: %s\n", path);
        loadConfig(config, file);
        fclose(file);
    }
}

void recursiveTestSearch(TestList* tests, const char* dir, TestCaseConfig* def) {
    struct stat file_stat;
    if(stat(dir, &file_stat) == 0) {
        if (file_stat.st_mode & S_IFDIR) {
            TestCaseConfig dir_def;
            copyTestConfig(&dir_def, def);
            searchForDefaultSettings(dir, &dir_def);
            DIR* directory = opendir(dir);
            if (directory != NULL) {
                struct dirent* file = readdir(directory);
                while (file != NULL) {
                    if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0) {
                        int dir_length = strlen(dir);
                        int file_length = strlen(file->d_name);
                        char path[dir_length + file_length + 2];
                        memcpy(path, dir, dir_length);
                        path[dir_length] = '/';
                        memcpy(path + dir_length + 1, file->d_name, file_length);
                        path[dir_length + file_length + 1] = 0;
                        recursiveTestSearch(tests, path, &dir_def);
                    }
                    file = readdir(directory);
                }
                closedir(directory);
            }
            deinitTestConfig(&dir_def);
        } else if (file_stat.st_mode & S_IFREG) {
            FILE* file = fopen(dir, "r");
            if (file != NULL) {
                makeSpaceInTestList(tests);
                if (tryToLoadTest(&tests->tests[tests->count], def, file)) {
                    tests->tests[tests->count].path = copyString(dir);
                    fprintf(stderr, "test: %s: %s\n", tests->tests[tests->count].path, tests->tests[tests->count].config.name);
                    tests->count++;
                }
                fclose(file);
            }
        }
    }
}
