
#include "testcase.h"
#include "testsearch.h"
#include "testrun.h"
#include "testprint.h"

int main(int argc, char** argv) {
    TestList tests;
    initTestList(&tests);
    TestCaseConfig def;
    initTestConfig(&def);
    recursiveTestSearch(&tests, "./gitignore.examples", &def);
    deinitTestConfig(&def);
    runTests(&tests, 1);
    printAllTestResults(&tests, stdout);
    deinitTestList(&tests);
    return 0;
}
