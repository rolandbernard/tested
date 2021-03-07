#ifndef _TESTCASE_H_
#define _TESTCASE_H_

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
    bool times_out_build;
    bool times_out;
    ConstraintList buildtime;
    ConstraintList time;
    ConstraintList exit;
    ConstraintList err;
    ConstraintList out;
} TestCaseConfig;

typedef struct {
    bool completed;
    bool failed;
    bool unsatisfiable;
    bool failed_build;
    bool failed_cleanup;
    long buildtime;
    bool out_of_buildtime;
    long runtime;
    bool out_of_runtime;
    int exit;
    int signal;
    char* err;
    char* out;
} TestCaseResult;

typedef struct {
    char* path;
    TestCaseConfig config;
    TestCaseResult result;
} TestCase;

typedef struct {
    TestCase* tests;
    int count;
    int capacity;
} TestList;

void initTestList(TestList* tests);

void makeSpaceInTestList(TestList* tests);

void deinitTestList(TestList* tests);

void initTestResult(TestCaseResult* result);

void initTestConfig(TestCaseConfig* test);

void copyTestConfig(TestCaseConfig* dst, TestCaseConfig* src);

void deinitTestConfig(TestCaseConfig* test);

void deinitTest(TestCase* test);

#endif