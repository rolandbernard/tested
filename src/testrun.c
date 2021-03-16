
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/ioctl.h>
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

static char* readStringFromFd(int fd) {
    int length = 0;
    ioctl(fd, FIONREAD, &length); 
    char* buffer = (char*)malloc(length + 1);
    if (length > 0) {
        read(fd, buffer, length);
    }
    buffer[length] = 0;
    return buffer;
}

typedef enum {
    TEST_EMPTY = 0,
    TEST_RUNNING_BUILD,
    TEST_TO_FINISHED_BUILD,
    TEST_FINISHED_BUILD,
    TEST_RUNNING_RUN,
    TEST_TO_FINISHED_RUN,
    TEST_FINISHED_RUN,
    TEST_RUNNING_CLEANUP,
    TEST_TO_FINISHED_CLEANUP,
    TEST_FINISHED_CLEANUP,
} TestRunState;

typedef struct {
    TestCase* test;
    int run;
    TestRunState state;
    long timeout;
    bool timed_out;
    pid_t pid;
    struct timeval start_time;
    struct timeval end_time;
    int status;
    int pipes[3][2];
} TestRunStatus;

static void startRunningCommand(TestRunStatus* test_run, const char* command, const char* in, long timeout) {
    test_run->timeout = timeout;
    test_run->timed_out = false;
    int pipes[3][2];
    pipe(pipes[0]);
    pipe(pipes[1]);
    pipe(pipes[2]);
    if (in != NULL) {
        write(pipes[0][1], in, strlen(in));
    }
    gettimeofday(&test_run->start_time, NULL);
    int pid = fork();
    if (pid < 0) {
        perror("Failed to fork");
    } else if (pid == 0) {
        setsid();
        dup2(pipes[0][0], fileno(stdin));
        dup2(pipes[1][1], fileno(stdout));
        dup2(pipes[2][1], fileno(stderr));
        execlp("sh", "sh", "-c", command, NULL);
        exit(EXIT_FAILURE);
    } else {
        test_run->pid = pid;
        memcpy(test_run->pipes, pipes, sizeof(pipes));
    }
}

static void stopCommand(TestRunStatus* test_run, long* elapsed, bool* timedout, int* exitc, int* termsig, char** err, char** out) {
    test_run->pid = 0;
    if (WIFEXITED(test_run->status)) {
        *exitc = WEXITSTATUS(test_run->status);
        *termsig = 0;
    } else if (WIFSIGNALED(test_run->status)) {
        *exitc = 0;
        *termsig = WTERMSIG(test_run->status);
    } else if (WIFSTOPPED(test_run->status)) {
        *exitc = 0;
        *termsig = WSTOPSIG(test_run->status);
    }
    if (timedout != NULL) {
        *timedout = test_run->timed_out;
    }
    if (elapsed != NULL) {
        *elapsed = (test_run->end_time.tv_sec - test_run->start_time.tv_sec) * 1000000 + (test_run->end_time.tv_usec - test_run->start_time.tv_usec);
    }
    close(test_run->pipes[0][1]);
    close(test_run->pipes[1][1]);
    close(test_run->pipes[2][1]);
    if (err != NULL) {
        *err = readStringFromFd(test_run->pipes[2][0]);
    }
    if (out != NULL) {
        *out = readStringFromFd(test_run->pipes[1][0]);
    }
    close(test_run->pipes[0][0]);
    close(test_run->pipes[1][0]);
    close(test_run->pipes[2][0]);
}

static void startTest(TestRunStatus* test_run, TestCase* test) {
    test->result.unsatisfiable = !areIntConstraintsSatisfiable(&test->config.buildtime)
        || !areIntConstraintsSatisfiable(&test->config.time)
        || !areIntConstraintsSatisfiable(&test->config.exit)
        || !areStringConstraintsSatisfiable(&test->config.err)
        || !areStringConstraintsSatisfiable(&test->config.out);
    test->result.failed = test->result.unsatisfiable;
    test_run->test = test;
    test_run->run = 0;
    if (test->result.failed) {
        test_run->state = TEST_FINISHED_CLEANUP;
    }
}

static void startTestBuild(TestRunStatus* test_run) {
    TestCase* test = test_run->test;
    if (test->result.failed) {
        test_run->status = TEST_FINISHED_RUN;
    } else if (strlen(test->config.build_command) != 0) {
        test_run->state = TEST_RUNNING_BUILD;
        long timeout = getIntConstraintMaximum(&test->config.buildtime);
        if (timeout != LONG_MAX) {
            timeout += 10000;
        } else {
            timeout = -1;
        }
        char* command = fillFileNamePattern(test->config.build_command, test->path);
        startRunningCommand(test_run, command, NULL, timeout);
        free(command);
    } else {
        test_run->state = TEST_FINISHED_BUILD;
    }
}

static void stopTestBuild(TestRunStatus* test_run) {
    TestCase* test = test_run->test;
    stopCommand(test_run,
        &test->result.buildtime, &test->result.out_of_buildtime,
        &test->result.buildexit, &test->result.buildsignal, NULL, NULL
    );
    if (test->config.times_out_build) {
        if (!test->result.out_of_buildtime) {
            test->result.failed_build = true;
            test->result.failed = true;
        }
    } else if (
        testAllIntConstraints(&test->config.buildtime, test->result.buildtime) != NULL
        || test->result.buildexit != 0 || test->result.buildsignal != 0
    ) {
        test->result.failed_build = true;
        test->result.failed = true;
    }
    test_run->state = TEST_FINISHED_BUILD;
}

static void startTestRun(TestRunStatus* test_run) {
    TestCase* test = test_run->test;
    if (test->result.failed) {
        test_run->status = TEST_FINISHED_RUN;
    } else if (strlen(test->config.run_command) != 0) {
        test_run->state = TEST_RUNNING_RUN;
        long timeout = getIntConstraintMaximum(&test->config.time);
        if (timeout != LONG_MAX) {
            timeout += 10000;
        } else {
            timeout = -1;
        }
        char* command = fillFileNamePattern(test->config.run_command, test->path);
        startRunningCommand(test_run, command, test->config.in, timeout);
        free(command);
    } else {
        test_run->state = TEST_FINISHED_RUN;
    }
}

static void stopTestRun(TestRunStatus* test_run) {
    TestCase* test = test_run->test;
    free(test->result.err);
    free(test->result.out);
    stopCommand(test_run,
        &test->result.runtime, &test->result.out_of_runtime, &test->result.exit,
        &test->result.signal, &test->result.err, &test->result.out
    );
    if (test->config.times_out) {
        if (!test->result.out_of_runtime) {
            test->result.failed = true;
        }
    } else if (
        testAllIntConstraints(&test->config.time, test->result.runtime) != NULL
        || testAllIntConstraints(&test->config.exit, test->result.exit) != NULL
        || testAllStringConstraints(&test->config.err, test->result.err) != NULL
        || testAllStringConstraints(&test->config.out, test->result.out) != NULL
        || test->result.signal != 0
    ) {
        test->result.failed = true;
    }
    test_run->state = TEST_FINISHED_RUN;
}

static void startTestCleanup(TestRunStatus* test_run) {
    TestCase* test = test_run->test;
    if (strlen(test->config.build_command) != 0) {
        test_run->state = TEST_RUNNING_CLEANUP;
        char* command = fillFileNamePattern(test->config.cleanup_command, test->path);
        startRunningCommand(test_run, command, test->config.in, -1);
        free(command);
    } else {
        test_run->state = TEST_FINISHED_CLEANUP;
    }
}

static void stopTestCleanup(TestRunStatus* test_run) {
    TestCase* test = test_run->test;
    stopCommand(test_run, NULL, NULL, &test->result.cleanupexit, &test->result.cleanupsignal, NULL, NULL);
    if (test->result.cleanupexit != 0 || test->result.cleanupsignal != 0) {
        test->result.failed_cleanup = true;
        test->result.failed = true;
    }
    test_run->state = TEST_FINISHED_CLEANUP;
}

static void testForTimeout(TestRunStatus* test_run) {
    struct timeval time;
    gettimeofday(&time, NULL);
    long elapsed = (time.tv_sec - test_run->start_time.tv_sec) * 1000000 + (time.tv_usec - test_run->start_time.tv_usec);
    if (test_run->timeout > 0 && elapsed > test_run->timeout) {
        kill(-test_run->pid, SIGKILL);
        test_run->timed_out = true;
    }
}

TestRunStatus* running_tests;
int running_test_count;

static void collectChilds() {
    struct timeval time;
    gettimeofday(&time, NULL);
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < running_test_count; i++) {
            if (running_tests[i].pid == pid) {
                running_tests[i].status = status;
                running_tests[i].end_time = time;
                switch (running_tests[i].state) {
                case TEST_RUNNING_BUILD:
                    running_tests[i].state = TEST_TO_FINISHED_BUILD;
                    break;
                case TEST_RUNNING_RUN:
                    running_tests[i].state = TEST_TO_FINISHED_RUN;
                    break;
                case TEST_RUNNING_CLEANUP:
                    running_tests[i].state = TEST_TO_FINISHED_CLEANUP;
                    break;
                default:
                    break;
                }
                break;
            }
        }
    }
}

static void killChilds() {
    for (int i = 0; i < running_test_count; i++) {
        switch (running_tests[i].state) {
        case TEST_RUNNING_BUILD:
        case TEST_RUNNING_RUN:
        case TEST_RUNNING_CLEANUP:
            kill(-running_tests[i].pid, SIGKILL);
            break;
        default:
            break;
        }
    }
}

static void signalHandler(int signal) {
    if (signal == SIGCHLD) {
        collectChilds();
    } else {
        killChilds();
        exit(0);
    }
}

static void printState(TestRunStatus* test_runs, int jobs, int passed, int failed) {
    fputc('\n', stderr);
    fprintf(stderr, "\e[32mtests passed: %i\e[m\n", passed);
    fprintf(stderr, "\e[31mtests failed: %i\e[m\n", failed);
    fputc('\n', stderr);
    for (int i = 0; i < jobs; i++) {
        if (test_runs[i].state == TEST_EMPTY) {
            fprintf(stderr, "task %i: IDLE\n", i);
        } else {
            if (test_runs[i].test->config.run_count > 1) {
                fprintf(stderr, "task %i: %s [%i]\n", i, test_runs[i].test->config.name, test_runs[i].run);
            } else {
                fprintf(stderr, "task %i: %s\n", i, test_runs[i].test->config.name);
            }
        }
    }
    fputc('\n', stderr);
}

void runTests(TestList* tests, int jobs, bool progress) {
    bool istty = isatty(fileno(stderr));
    TestRunStatus test_runs[jobs];
    for (int i = 0; i < jobs; i++) {
        test_runs[i].state = TEST_EMPTY;
        test_runs[i].pid = 0;
    }
    running_tests = test_runs;
    running_test_count = jobs;
    signal(SIGCHLD, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    // run tests
    int tests_enqueued = 0;
    int tests_run = 0;
    int tests_failed = 0;
    if (progress && istty) {
        printState(test_runs, jobs, tests_run - tests_failed, tests_failed);
    }
    struct timespec sleep = { .tv_sec = 0, .tv_nsec = 1000000 };
    bool prev_change = false;
    while (tests_run < tests->count) {
        collectChilds();
        bool state_changed = false;
        for (int i = 0; i < jobs; i++) {
            switch (test_runs[i].state) {
            case TEST_EMPTY: {
                if (tests_enqueued < tests->count) {
                    startTest(&test_runs[i], &tests->tests[tests_enqueued]);
                    tests_enqueued++;
                    startTestBuild(&test_runs[i]);
                    state_changed = true;
                }
                break;
            }
            case TEST_RUNNING_BUILD: {
                testForTimeout(&test_runs[i]);
                break;
            }
            case TEST_TO_FINISHED_BUILD: {
                stopTestBuild(&test_runs[i]);
                state_changed = true;
                break;
            }
            case TEST_FINISHED_BUILD: {
                startTestRun(&test_runs[i]);
                state_changed = true;
                break;
            }
            case TEST_RUNNING_RUN: {
                testForTimeout(&test_runs[i]);
                break;
            }
            case TEST_TO_FINISHED_RUN: {
                stopTestRun(&test_runs[i]);
                state_changed = true;
                break;
            }
            case TEST_FINISHED_RUN: {
                startTestCleanup(&test_runs[i]);
                state_changed = true;
                break;
            }
            case TEST_RUNNING_CLEANUP: {
                break;
            }
            case TEST_TO_FINISHED_CLEANUP: {
                stopTestCleanup(&test_runs[i]);
                state_changed = true;
                break;
            }
            case TEST_FINISHED_CLEANUP: {
                test_runs[i].run++;
                if (!test_runs[i].test->result.failed && test_runs[i].test->config.run_count > test_runs[i].run) {
                    startTestBuild(&test_runs[i]);
                    state_changed = true;
                } else {
                    if (test_runs[i].test->result.failed) {
                        tests_failed++;
                    }
                    tests_run++;
                    test_runs[i].test->result.completed = true;
                    if (tests_enqueued < tests->count) {
                        startTest(&test_runs[i], &tests->tests[tests_enqueued]);
                        tests_enqueued++;
                        startTestBuild(&test_runs[i]);
                        state_changed = true;
                    } else {
                        test_runs[i].state = TEST_EMPTY;
                    }
                }
                break;
            }
            }
        }
        if (!state_changed) {
            if (progress && istty && prev_change) {
                fprintf(stderr, "\e[%iA\e[J", 5 + jobs);
                printState(test_runs, jobs, tests_run - tests_failed, tests_failed);
            }
            nanosleep(&sleep, NULL);
        }
        prev_change = state_changed;
    }
    if (progress && istty) {
        fprintf(stderr, "\e[%iA\e[J", 5 + jobs);
    }
    signal(SIGCHLD, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
}
