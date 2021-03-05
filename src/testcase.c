
#include <stdlib.h>

#include "testcase.h"

void runTest(TestCase* test) {

}

void freeTest(TestCase* test) {
    freeStringConstraints(&test->settings.err);
    freeStringConstraints(&test->settings.out);
    free(test->result.err);
    free(test->result.out);
}
