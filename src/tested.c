
#include "testsearch.h"

int main(int argc, char** argv) {
    TestList tests;
    initTestList(&tests);
    TestCaseConfig def;
    initTestConfig(&def);
    recursiveTestSearch(&tests, "./gitignore.examples", &def);
    deinitTestList(&tests);
    return 0;
}
