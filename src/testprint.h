#ifndef _TESTPRINT_H_
#define _TESTPRINT_H_

#include <stdio.h>

#include "testcase.h"

void printTestResult(TestCase* test, FILE* output, bool verbose);

void printTestResults(TestList* tests, FILE* output, bool all, bool verbose);

void printTestSummary(TestList* tests, FILE* output);

#endif