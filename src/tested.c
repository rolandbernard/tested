
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"
#include "testsearch.h"
#include "testload.h"
#include "testrun.h"
#include "testprint.h"

typedef struct {
    int num_jobs;
    bool all;
    bool verbose;
    bool progress;
    bool tasks;
} TestedArgs;

static void printHelp(const char* prog) {
    fprintf(stderr, "Usage: %s [options] test...\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -c --config=CONFIG\n");
    fprintf(stderr, "  -j --jobs=JOBS      number of parallel threads\n");
    fprintf(stderr, "  -h --help           print this text\n");
    fprintf(stderr, "  -v --verbose        print more information\n");
    fprintf(stderr, "  -a --all            print informations for all tests\n");
    fprintf(stderr, "  -P --no-progress    do not print progress information\n");
    fprintf(stderr, "  -t --print-tasks          show all currently running tasks\n");
}

static void initDefaultArgs(TestedArgs* args) {
    args->num_jobs = 1;
    args->all = false;
    args->verbose = false;
    args->progress = true;
    args->tasks = false;
}

static void parseArguments(int argc, const char** argv, TestCaseConfig* def, TestList* tests, TestedArgs* args) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == '-') {
                if (strncmp(argv[i], "--config", 8) == 0) {
                    if (argv[i][8] == '=') {
                        FILE* file = fopen(argv[i] + 9, "r");
                        if (file != NULL) {
                            loadConfig(def, file);
                            fclose(file);
                        } else {
                            fprintf(stderr, "failed to open config %s: %s\n", argv[i] + 9, strerror(errno));
                        }
                    } else if (argv[i][8] == 0) {
                        i++;
                        FILE* file = fopen(argv[i], "r");
                        if (file != NULL) {
                            loadConfig(def, file);
                            fclose(file);
                        } else {
                            fprintf(stderr, "failed to open config %s: %s\n", argv[i], strerror(errno));
                        }
                    } else {
                        fprintf(stderr, "unknown option %s\n", argv[i]);
                    }
                } else if (strncmp(argv[i], "--jobs", 6) == 0) {
                    if (argv[i][6] == '=') {
                        int jobs = 0;
                        for (int j = 0; argv[i][j + 7] != 0; j++) {
                            if (argv[i][j + 7] >= '0' && argv[i][j + 7] <= '9') {
                                jobs *= 10;
                                jobs += argv[i][j + 7] - '0';
                            } else {
                                fprintf(stderr, "job count must be an integer\n");
                                break;
                            }
                        }
                        args->num_jobs = jobs;
                    } else if (argv[i][6] == 0) {
                        i++;
                        int jobs = 0;
                        for (int j = 0; argv[i][j] != 0; j++) {
                            if (argv[i][j] >= '0' && argv[i][j] <= '9') {
                                jobs *= 10;
                                jobs += argv[i][j] - '0';
                            } else {
                                fprintf(stderr, "job count must be an integer\n");
                                break;
                            }
                        }
                        args->num_jobs = jobs;
                    } else {
                        fprintf(stderr, "unknown option %s\n", argv[i]);
                    }
                } else if (strcmp(argv[i], "--help") == 0) {
                    printHelp(argv[0]);
                } else if (strcmp(argv[i], "--verbose") == 0) {
                    args->verbose = true;
                } else if (strcmp(argv[i], "--no-progress") == 0) {
                    args->progress = false;
                } else if (strcmp(argv[i], "--print-tasks") == 0) {
                    args->tasks = true;
                } else if (strcmp(argv[i], "--all") == 0) {
                    args->all = true;
                } else {
                    fprintf(stderr, "unknown option %s\n", argv[i]);
                }
            } else {
                int p = i;
                for (int j = 1; argv[p][j] != 0; j++) {
                    if (argv[p][j] == 'c') {
                        i++;
                        FILE* file = fopen(argv[i], "r");
                        if (file != NULL) {
                            loadConfig(def, file);
                            fclose(file);
                        } else {
                            fprintf(stderr, "failed to open config %s: %s\n", argv[i], strerror(errno));
                        }
                    } else if (argv[p][j] == 'j') {
                        if (argv[p][j + 1] >= '0' && argv[p][j + 1] <= '9') {
                            int jobs = 0;
                            while (argv[p][j + 1] >= '0' && argv[p][j + 1] <= '9') {
                                jobs *= 10;
                                jobs += argv[p][j + 1] - '0';
                                j++;
                            }
                            args->num_jobs = jobs;
                        } else {
                            i++;
                            int jobs = 0;
                            for (int j = 0; argv[i][j] != 0; j++) {
                                if (argv[i][j] >= '0' && argv[i][j] <= '9') {
                                    jobs *= 10;
                                    jobs += argv[i][j] - '0';
                                } else {
                                    fprintf(stderr, "job count must be an integer\n");
                                    break;
                                }
                            }
                            args->num_jobs = jobs;
                        }
                    } else if (argv[p][j] == 'h') {
                        printHelp(argv[0]);
                    } else if (argv[p][j] == 'v') {
                        args->verbose = true;
                    } else if (argv[p][j] == 'P') {
                        args->progress = false;
                    } else if (argv[p][j] == 't') {
                        args->tasks = true;
                    } else if (argv[p][j] == 'a') {
                        args->all = true;
                    } else {
                        fprintf(stderr, "unknown option -%c\n", argv[p][j]);
                    }
                }
            }
        } else {
            recursiveTestSearch(tests, argv[i], def);
        }
    }
}

int main(int argc, const char** argv) {
    TestedArgs args;
    initDefaultArgs(&args);
    TestList tests;
    initTestList(&tests);
    TestCaseConfig def;
    initTestConfig(&def);
    parseArguments(argc, argv, &def, &tests, &args);
    deinitTestConfig(&def);
    if (tests.count > 0) {
        runTests(&tests, args.num_jobs, args.progress, args.tasks);
        printTestResults(&tests, stdout, args.all, args.verbose);
    }
    deinitTestList(&tests);
    return 0;
}

