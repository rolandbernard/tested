#ifndef _TESTPRINT_H_
#define _TESTPRINT_H_

#include <stdio.h>

#include "testcase.h"

void printTestResult(TestCase* test, FILE* output);

void printTestResults(TestList* tests, FILE* output);

void printTestSummary(TestList* tests, FILE* output);

#endif