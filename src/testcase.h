#ifndef _TESTRUN_H_
#define _TESTRUN_H_

#include <stdbool.h>

#include "constraint.h"

typedef struct {
    char* name;
    // instructions
    int run_count;
    char* build_command;
    char* run_command;
    char* cleanup_command;
    char* in;
    // constraints
    ConstraintList buildtime;
    ConstraintList buildcputime;
    ConstraintList time;
    ConstraintList cputime;
    ConstraintList exit;
    ConstraintList err;
    ConstraintList out;
} TestCaseSettings;

typedef struct {
    bool unsatisfiable;
    bool failed_build;
    bool failed_cleanup;
    int buildtime;
    bool out_of_buildtime;
    int runtime;
    bool out_of_runtime;
    int time;
    bool out_of_time;
    int cputime;
    bool out_of_cputime;
    int exit;
    char* err;
    char* out;
} TestCaseResult;

typedef struct {
    TestCaseSettings settings;
    TestCaseResult result;
} TestCase;

void runTest(TestCase* test);

void freeTest(TestCase* test);

#endif