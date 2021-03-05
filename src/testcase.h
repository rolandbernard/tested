#ifndef _TESTRUN_H_
#define _TESTRUN_H_

#include <stdbool.h>

#include "constraint.h"

typedef struct {
    char* name;
    // instructions
    long run_count;
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
} TestCaseConfig;

typedef struct {
    bool unsatisfiable;
    bool failed_build;
    bool failed_cleanup;
    long buildtime;
    bool out_of_buildtime;
    long runtime;
    bool out_of_runtime;
    long time;
    bool out_of_time;
    long cputime;
    bool out_of_cputime;
    long exit;
    char* err;
    char* out;
} TestCaseResult;

typedef struct {
    TestCaseConfig config;
    TestCaseResult result;
} TestCase;

void runTest(TestCase* test);

void initTestResult(TestCaseResult* result);

void initTestConfig(TestCaseConfig* test);

void copyTestConfig(TestCaseConfig* dst, TestCaseConfig* src);

void deinitTestConfig(TestCaseConfig* test);

void deinitTest(TestCase* test);

#endif