
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

void runTest(TestCase* test) {
    for (int i = 0; i < test->config.run_count; i++) {
        struct timespec smallsleep = {.tv_sec = 0, .tv_nsec = 1000000};
        if (strlen(test->config.build_command) != 0) {
            fprintf(stderr, "Building %s...\n", test->path);
            long cputime = min(getIntConstraintMaximum(&test->config.buildcputime), getIntConstraintMaximum(&test->config.buildtime));
            long time = getIntConstraintMaximum(&test->config.buildtime);
            char* command = fillFileNamePattern(test->config.build_command, test->path);
            struct timeval starttime;
            gettimeofday(&starttime, NULL);
            int pid = fork();
            if (pid == 0) {
                if (cputime != LONG_MAX) {
                    struct rlimit cpu_limit;
                    cpu_limit.rlim_cur = (cputime + 999999) / 1000000;
                    cpu_limit.rlim_max = (cputime + 999999) / 1000000;
                    setrlimit(RLIMIT_CPU, &cpu_limit);
                }
                execlp("sh", "sh", "-c", command, NULL);
                exit(EXIT_FAILURE);
            } else {
                int stat;
                struct timeval endtime;
                struct rusage before;
                getrusage(RUSAGE_CHILDREN, &before);
                while (waitpid(pid, &stat, WNOHANG) == 0) {
                    gettimeofday(&endtime, NULL);
                    if (((long)endtime.tv_usec - (long)starttime.tv_usec) + ((long)endtime.tv_sec - (long)starttime.tv_sec) * 1000000 > time) {
                        kill(pid, SIGKILL);
                    }
                    nanosleep(&smallsleep, NULL);
                }
                gettimeofday(&endtime, NULL);
                struct rusage after;
                getrusage(RUSAGE_CHILDREN, &after);
            }
            free(command);
        }
        if (strlen(test->config.run_command) != 0) {
            fprintf(stderr, "Running %s...\n", test->path);
            long cputime = min(getIntConstraintMaximum(&test->config.cputime), getIntConstraintMaximum(&test->config.time));
            long time = getIntConstraintMaximum(&test->config.time);
            char* command = fillFileNamePattern(test->config.run_command, test->path);
            struct timeval starttime;
            gettimeofday(&starttime, NULL);
            struct tms before;
            times(&before);
            int pid = fork();
            if (pid == 0) {
                if (cputime != LONG_MAX) {
                    struct rlimit cpu_limit;
                    cpu_limit.rlim_cur = (cputime + 999999) / 1000000;
                    cpu_limit.rlim_max = (cputime + 999999) / 1000000;
                    setrlimit(RLIMIT_CPU, &cpu_limit);
                }
                execlp("sh", "sh", "-c", command, NULL);
                exit(EXIT_FAILURE);
            } else {
                bool timed_out = false;
                int stat;
                struct timeval endtime;
                while (waitpid(pid, &stat, WNOHANG) == 0) {
                    gettimeofday(&endtime, NULL);
                    if (((long)endtime.tv_usec - (long)starttime.tv_usec) + ((long)endtime.tv_sec - (long)starttime.tv_sec) * 1000000 > time) {
                        kill(pid, SIGKILL);
                        int tmp;
                        timed_out = true;
                    }
                    nanosleep(&smallsleep, NULL);
                }
                gettimeofday(&endtime, NULL);
                struct tms after;
                times(&after);
                long time = ((after.tms_cutime - before.tms_cutime) * 1000000 + (after.tms_cstime - before.tms_cstime) * 1000000) / sysconf(_SC_CLK_TCK);
                fprintf(stderr, "time: %lgs\n", time / (double)1000000);
                fprintf(stderr, "befor: %lu %lu\n", before.tms_cutime, before.tms_cstime);
                fprintf(stderr, "after: %lu %lu\n", after.tms_cutime, after.tms_cstime);
            }
            free(command);
        }
        if (strlen(test->config.cleanup_command) != 0) {
            char* command = fillFileNamePattern(test->config.cleanup_command, test->path);
            fprintf(stderr, "Cleanup %s...\n", test->path);
            int pid = fork();
            if (pid == 0) {
                execlp("sh", "sh", "-c", command, NULL);
                exit(EXIT_FAILURE);
            } else {
                int stat;
                waitpid(pid, &stat, 0);
                struct rusage usage;
            }
            free(command);
        }
        fprintf(stderr, "Finished %s.\n", test->path);
    }
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
