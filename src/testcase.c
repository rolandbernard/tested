
#include <stdlib.h>

#include "testcase.h"

void runTest(TestCase* test) {

}

void freeTest(TestCase* test) {
    freeStringConstraints(&test->settings.stderr);
    freeStringConstraints(&test->settings.stdout);
    free(test->result.stderr);
    free(test->result.stdout);
}
