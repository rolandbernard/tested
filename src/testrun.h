#ifndef _TESTRUN_H_
#define _TESTRUN_H_

#include "testcase.h"

void runTest(TestCase* test);

void runTests(TestList* tests, int jobs, bool progress);

#endif