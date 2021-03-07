
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "testprint.h"

static const char* getConstraintStr(ConstraintKind kind) {
    switch (kind) {
    case CONSTRAIND_EQUAL:
        return "=";
        break;
    case CONSTRAIND_UNEQUAL:
        return "!=";
        break;
    case CONSTRAIND_LESS:
        return "<";
        break;
    case CONSTRAIND_LESS_EQUAL:
        return "<=";
        break;
    case CONSTRAIND_MORE:
        return ">";
        break;
    case CONSTRAIND_MORE_EQUAL:
        return ">=";
        break;
    default:
        return "?=";
        break;
    }
}

void printTestResult(TestCase* test, FILE* output) {
    bool istty = isatty(fileno(output));
    if (test->result.completed && test->result.failed) {
        if (istty) {
            fprintf(output, "\e[31mfailed:\e[m ");
        } else {
            fprintf(output, "failed: ");
        }
        fprintf(output, "%s", test->path);
        if (test->config.name[0] != 0) {
            fprintf(output, ": %s", test->config.name);
        }
        fputc('\n', output);
        if (test->result.unsatisfiable) {
            fprintf(stderr, "--> the constraints are not satisfiable\n");
        } else {
            if (test->result.failed_cleanup) {
                if (test->result.buildexit != 0) {
                    fprintf(stderr, "--> cleanup failed with non zero exit code %i\n", test->result.cleanupexit);
                } else {
                    fprintf(stderr, "--> cleanup terminated with signal %s\n", strsignal(test->result.cleanupsignal));
                }
            }
            Constraint* constr;
            constr = testAllIntConstraints(&test->config.buildtime, test->result.buildtime);
            if (test->result.out_of_buildtime) {
                fprintf(stderr, "--> build was killed after %lgs, but expected %s %lgs\n", test->result.buildtime / 1e6, getConstraintStr(constr->kind), constr->value / 1e6);
            } else {
                if (constr != NULL) {
                    fprintf(stderr, "--> build took %lgs, but expected %s %lgs\n", test->result.buildtime / 1e6, getConstraintStr(constr->kind), constr->value / 1e6);
                }
                if (test->result.failed_build) {
                    if (test->result.buildexit != 0) {
                        fprintf(stderr, "--> build failed with non zero exit code %i\n", test->result.buildexit);
                    } else {
                        fprintf(stderr, "--> build terminated with signal %s\n", strsignal(test->result.buildsignal));
                    }
                }
            }
            constr = testAllIntConstraints(&test->config.time, test->result.runtime);
            if (test->result.out_of_runtime) {
                fprintf(stderr, "--> run killed after %lgs, but expected %s %lgs\n", test->result.runtime / 1e6, getConstraintStr(constr->kind), constr->value / 1e6);
            } else if (!test->result.failed_build) {
                if (constr != NULL) {
                    fprintf(stderr, "--> run took %lgs, but expected %s %lgs\n", test->result.runtime / 1e6, getConstraintStr(constr->kind), constr->value / 1e6);
                }
                constr = testAllIntConstraints(&test->config.exit, test->result.exit);
                if (constr != NULL) {
                    fprintf(stderr, "--> run exited with %i, but expected %s %li\n", test->result.exit, getConstraintStr(constr->kind), constr->value);
                }
                constr = testAllStringConstraints(&test->config.err, test->result.err);
                if (constr != NULL) {
                    fprintf(stderr, "--> stderr is not as expected\n");
                }
                constr = testAllStringConstraints(&test->config.out, test->result.out);
                if (constr != NULL) {
                    fprintf(stderr, "--> stdout is not as expected\n");
                }
            }
        }
        fputc('\n', output);
    }
}

void printTestResults(TestList* tests, FILE* output) {
    printTestSummary(tests, output);
    for (int i = 0; i < tests->count; i++) {
        printTestResult(&tests->tests[i], output);
    }
}

void printTestSummary(TestList* tests, FILE* output) {
    bool istty = isatty(fileno(output));
    int num_passed = 0;
    int num_failed = 0;
    for (int i = 0; i < tests->count; i++) {
        if (tests->tests[i].result.completed) {
            if (tests->tests[i].result.failed) {
                num_failed++;
            } else {
                num_passed++;
            }
        }
    }
    fputc('\n', output);
    if (istty) {
        fprintf(output, "\e[32mtests passed: %i\e[m\n", num_passed);
        fprintf(output, "\e[31mtests failed: %i\e[m\n", num_failed);
    } else {
        fprintf(output, "tests passed: %i\n", num_passed);
        fprintf(output, "tests failed: %i\n", num_failed);
    }
    fputc('\n', output);
}
