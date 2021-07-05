#ifndef _TESTLOAD_H_
#define _TESTLOAD_H_

#include <stdio.h>

#include "testcase.h"

bool tryToLoadTest(TestCase* test, TestCaseConfig* def, FILE* file);

void loadConfig(TestCaseConfig* config, FILE* file);

#endif
