
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#include "testrun.h"

#include "util.h"

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
                len++;
                i++;
            }
        } else {
            ret[len] = command[i];
            len++;
        }
    }
    ret[len] = 0;
    return ret;
}

static void childSignal(int signal) { }

#define INITIAL_BUFFER_CAPACITY 64

static char* readStringFromFd(int fd) {
    int capacity = INITIAL_BUFFER_CAPACITY;
    int length = 0;
    char* buffer = (char*)malloc(capacity);
    int last_length = 0;
    do {
        last_length = read(fd, buffer + length, capacity - length);
        if (last_length > 0) {
            length += last_length;
            if (length == capacity) {
                capacity *= 2;
                buffer = (char*)realloc(buffer, capacity);
            }
        }
    } while (last_length > 0);
    buffer = (char*)realloc(buffer, length + 1);
    buffer[length] = 0;
    return buffer;
}

static void runTimedCommand(
    const char* command, long timeout, long* elapsed, bool* timedout,
    int* exitc, int* termsig, const char* in, char** err, char** out
) {
    struct timespec sleep;
    if (timeout >= 0) {
        sleep.tv_sec = timeout / 1000000;
        sleep.tv_nsec = (timeout * 1000) % 1000000000;
    } else {
        sleep.tv_sec = 60L * 60 * 24 * 356; // 1 years
        sleep.tv_nsec = 0;
    }
    int pipes[3][2];
    pipe(pipes[0]);
    pipe(pipes[1]);
    pipe(pipes[2]);
    signal(SIGCHLD, childSignal);
    if (in != NULL) {
        write(pipes[0][1], in, strlen(in));
    }
    int pid = fork();
    if (pid == 0) {
        setsid();
        dup2(pipes[0][0], fileno(stdin));
        dup2(pipes[1][1], fileno(stdout));
        dup2(pipes[2][1], fileno(stderr));
        execlp("sh", "sh", "-c", command, NULL);
        exit(EXIT_FAILURE);
    } else {
        bool timed_out = false;
        struct timespec endtime = {.tv_sec = 0, .tv_nsec = 0};
        if (nanosleep(&sleep, &endtime) == 0) {
            kill(-pid, SIGKILL);
            timed_out = true;
        }
        int stat;
        waitpid(pid, &stat, 0);
        if (timedout != NULL) {
            *timedout = timed_out;
        }
        if (elapsed != NULL) {
            *elapsed = (sleep.tv_sec - endtime.tv_sec) * 1000000 + (sleep.tv_nsec - endtime.tv_nsec) / 1000;
            fprintf(stderr, "time: %lgs\n", *elapsed / (double)1000000);
        }
        close(pipes[0][1]);
        close(pipes[1][1]);
        close(pipes[2][1]);
        if (err != NULL) {
            *err = readStringFromFd(pipes[2][0]);
            fprintf(stderr, "err: %s\n", *err);
        }
        if (out != NULL) {
            *out = readStringFromFd(pipes[1][0]);
            fprintf(stderr, "out: %s\n", *out);
        }
        close(pipes[0][0]);
        close(pipes[1][0]);
        close(pipes[2][0]);
    }
}

void runTest(TestCase* test) {
    test->result.unsatisfiable = !areIntConstraintsSatisfiable(&test->config.buildtime)
        || !areIntConstraintsSatisfiable(&test->config.time)
        || !areIntConstraintsSatisfiable(&test->config.exit);
    test->result.failed = test->result.unsatisfiable;
    for (int i = 0; !test->result.failed && i < test->config.run_count; i++) {
        if (strlen(test->config.build_command) != 0) {
            long timeout = getIntConstraintMaximum(&test->config.buildtime);
            if (timeout != LONG_MAX) {
                timeout += 10000;
            } else {
                timeout = -1;
            }
            char* command = fillFileNamePattern(test->config.build_command, test->path);
            int exited;
            int signaled;
            runTimedCommand(
                command, timeout,
                &test->result.buildtime, &test->result.out_of_buildtime,
                &exited, &signaled, NULL, NULL, NULL
            );
            free(command);
            if (
                testAllIntConstraints(&test->config.buildtime, test->result.buildtime) != NULL
                || exited != 0 || signaled != 0
            ) {
                test->result.failed_build = true;
                test->result.failed = true;
            }
        }
        if (!test->result.failed) {
            if (strlen(test->config.run_command) != 0) {
                long timeout = getIntConstraintMaximum(&test->config.time);
                if (timeout != LONG_MAX) {
                    timeout += 10000;
                } else {
                    timeout = -1;
                }
                char* command = fillFileNamePattern(test->config.run_command, test->path);
                runTimedCommand(
                    command, timeout,
                    &test->result.runtime, &test->result.out_of_runtime,
                    &test->result.exit, &test->result.signal,
                    test->config.in, &test->result.err, &test->result.out
                );
                free(command);
                if (
                    testAllIntConstraints(&test->config.time, test->result.runtime) != NULL
                    || testAllIntConstraints(&test->config.exit, test->result.exit) != NULL
                    || testAllStringConstraints(&test->config.err, test->result.err) != NULL
                    || testAllStringConstraints(&test->config.out, test->result.out) != NULL
                ) {
                    test->result.failed = true;
                }
            }
        }
        if (strlen(test->config.cleanup_command) != 0) {
            char* command = fillFileNamePattern(test->config.cleanup_command, test->path);
            int exited;
            int signaled;
            runTimedCommand(command, -1, NULL, NULL, &exited, &signaled, NULL, NULL, NULL);
            free(command);
            if (exited != 0 || signaled != 0) {
                test->result.failed_cleanup = true;
            }
        }
    }
    test->result.completed = true;
}

typedef struct {
    pthread_t thread;
    int id;
    int num_jobs;
    TestList* tests;
} TestRunnerData;

void* testRunner(void* udata) {
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
    }
    if (jobs == 1) {
        testRunner((void*)test_runners);
    } else {
        for (int i = 0; i < jobs; i++) {
            pthread_create(&test_runners[i].thread, NULL, testRunner, &test_runners[i]);
        }
        for (int i = 0; i < jobs; i++) {
            pthread_join(test_runners[i].thread, NULL);
        }
    }
}
