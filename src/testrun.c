
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

#include "testrun.h"

static char* fillFileNamePattern(const char* command, const char* path) {
    int num_patterns = 0;
    for (int i = 0; command[i] != 0; i++) {
        if (command[i] == '%') {
            if (command[i + 1] != '%') {
                num_patterns++;
            } else {
                i++;
            }
        }
    }
    int len_command = strlen(command);
    int len_path = strlen(path);
    char* ret = (char*)malloc(len_command + num_patterns * len_path + 1);
    int len = 0;
    for (int i = 0; i < len_command; i++) {
        if (command[i] == '%') {
            if (command[i + 1] != '%') {
                memcpy(ret + len, path, len_path);
                len += len_path;
            } else {
                ret[len] = '%';
                i++;
            }
        } else {
            ret[len] = command[i];
        }
    }
    ret[len] = 0;
    return ret;
}

void runTest(TestCase* test) {
    int pid = fork();
    if (pid == 0) {
        char* command = fillFileNamePattern(test->config.build_command, test->path);
        execlp("sh", "sh", "-c", command, NULL);
        free(command);
        exit(EXIT_FAILURE);
    } else {
        int stat;
        waitpid(pid, &stat, 0);
    }
    pid = fork();
    if (pid == 0) {
        char* command = fillFileNamePattern(test->config.run_command, test->path);
        fprintf(stderr, "%s\n", command);
        execlp("sh", "sh", "-c", command, NULL);
        free(command);
        exit(EXIT_FAILURE);
    } else {
        int stat;
        waitpid(pid, &stat, 0);
    }
    pid = fork();
    if (pid == 0) {
        char* command = fillFileNamePattern(test->config.cleanup_command, test->path);
        execlp("sh", "sh", "-c", command, NULL);
        free(command);
        exit(EXIT_FAILURE);
    } else {
        int stat;
        waitpid(pid, &stat, 0);
    }
}

typedef struct {
    pthread_t thread;
    int id;
    int num_jobs;
    TestList* tests;
} TestRunnerData;

void* tastRunner(void* udata) {
    TestRunnerData* data = (TestRunnerData*)udata;
    for (int t = data->id; t < data->tests->count; t += data->num_jobs) {
        runTest(&data->tests->tests[t]);
    }
    return NULL;
}

void runTests(TestList* tests, int jobs) {
    TestRunnerData test_runners[jobs];
    for (int i = 0; i < jobs; i++) {
        test_runners[i].id = i;
        test_runners[i].num_jobs = jobs;
        test_runners[i].tests = tests;
        pthread_create(&test_runners[i].thread, NULL, tastRunner, &test_runners[i]);
    }
    for (int i = 0; i < jobs; i++) {
        pthread_join(test_runners[i].thread, NULL);
    }
}
