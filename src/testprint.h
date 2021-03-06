#ifndef _TESTPRINT_H_
#define _TESTPRINT_H_

#include <stdio.h>

#include "testcase.h"

void printTestResults(TestCase* test, FILE* output);

void printAllTestResults(TestList* tests, FILE* output);

#endif