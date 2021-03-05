#ifndef _TESTLOAD_H_
#define _TESTLOAD_H_

#include <stdio.h>

#include "testcase.h"

bool tryToLoadTest(TestCase* test, TestCaseSettings* def, FILE* file);

void loadConfig(TestCaseSettings* config, FILE* file);

#endif