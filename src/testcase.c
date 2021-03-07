
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "testcase.h"

#include "util.h"

void initTestResult(TestCaseResult* result) {
    result->completed = false;
    result->failed = false;
    result->unsatisfiable = false;
    result->failed_build = false;
    result->failed_cleanup = false;
    result->buildtime = 0;
    result->out_of_buildtime = false;
    result->runtime = 0;
    result->out_of_runtime = false;
    result->exit = 0;
    result->signal = 0;
    result->err = NULL;
    result->out = NULL;
}

void initTestConfig(TestCaseConfig* test) {
    test->name = copyString("");
    test->run_count = 1;
    test->build_command = copyString("");
    test->run_command = copyString("");
    test->cleanup_command = copyString("");
    test->in = copyString("");
    test->buildtime.count = 0;
    test->times_out = false;
    test->times_out_build = false;
    test->time.count = 0;
    test->exit.count = 0;
    test->err.count = 0;
    test->out.count = 0;
}

void copyTestConfig(TestCaseConfig* dst, TestCaseConfig* src) {
    *dst = *src;
    dst->name = copyString(src->name);
    dst->build_command = copyString(src->build_command);
    dst->run_command = copyString(src->run_command);
    dst->cleanup_command = copyString(src->cleanup_command);
    dst->in = copyString(src->in);
    copyStringConstraints(&dst->err, &src->err);
    copyStringConstraints(&dst->out, &src->out);
}

void deinitTestConfig(TestCaseConfig* test) {
    free(test->name);
    free(test->build_command);
    free(test->run_command);
    free(test->cleanup_command);
    free(test->in);
    deinitStringConstraints(&test->err);
    deinitStringConstraints(&test->out);
}

void deinitTest(TestCase* test) {
    free(test->path);
    deinitTestConfig(&test->config);
    free(test->result.err);
    free(test->result.out);
}

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

void makeSpaceInTestList(TestList* tests) {
    if (tests->count == tests->capacity) {
        if (tests->capacity == 0) {
            tests->capacity = INITIAL_TEST_LIST_CAPACITY;
        } else {
            tests->capacity *= 2;
        }
        tests->tests = (TestCase*)realloc(tests->tests, sizeof(TestCase) * tests->capacity);
    }
}
