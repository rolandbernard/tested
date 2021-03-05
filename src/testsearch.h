#ifndef _TESTSEARCH_H_
#define _TESTSEARCH_H_

#include "testcase.h"

typedef struct {
    TestCase* tests;
    int count;
    int capacity;
} TestList;

void initTestList(TestList* tests);

void deinitTestList(TestList* tests);

void recursiveTestSearch(TestList* tests, const char* start, TestCaseConfig* def);

#endif