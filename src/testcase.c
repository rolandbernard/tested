
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "testcase.h"

void runTest(TestCase* test) {

}

void initTestResult(TestCaseResult* result) {
    result->err = NULL;
    result->out = NULL;
}

void initTestConfig(TestCaseConfig* test) {
    test->name = "";
    test->run_count = 1;
    test->build_command = "";
    test->run_command = "";
    test->cleanup_command = "";
    test->in = "";
    test->buildtime.count = 0;
    test->buildcputime.count = 0;
    test->time.count = 0;
    test->cputime.count = 0;
    test->exit.count = 0;
    test->err.count = 0;
    test->out.count = 0;
}

static char* copyString(const char* str) {
    int length = strlen(str);
    char* ret = (char*)malloc(length + 1);
    memcpy(ret, str, length + 1);
    return ret;
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
    deinitTestConfig(&test->config);
    free(test->result.err);
    free(test->result.out);
}
