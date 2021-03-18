
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

void printTestResult(TestCase* test, FILE* output, bool verbose) {
    bool istty = isatty(fileno(output));
    if (istty) {
        if (test->result.failed) {
            fprintf(output, "\e[31mfailed:\e[m ");
        } else if (test->result.completed) {
            fprintf(output, "\e[32mpassed:\e[m ");
        } else {
            fprintf(output, "\e[33mincomplete:\e[m ");
        }
    } else {
        if (test->result.failed) {
            fprintf(output, "failed: ");
        } else if (test->result.completed) {
            fprintf(output, "passed: ");
        } else {
            fprintf(output, "incomplete: ");
        }
    }
    fprintf(output, "%s", test->path);
    if (test->config.name[0] != 0) {
        fprintf(output, ": %s", test->config.name);
    }
    fputc('\n', output);
    if (test->result.completed) {
        if (test->result.unsatisfiable) {
            fprintf(output, "--> the constraints are not satisfiable\n");
            if (verbose) {
                fprintf(output, "==> stopped the run\n");
            }
        } else {
            if (strlen(test->config.build_command) != 0) {
                Constraint* constr;
                constr = testAllIntConstraints(&test->config.buildtime, test->result.buildtime);
                if (test->result.out_of_buildtime) {
                    fprintf(output, "--> build was killed after %lgs, but expected %s %lgs\n", test->result.buildtime / 1e6, getConstraintStr(constr->kind), constr->value / 1e6);
                } else {
                    if (constr != NULL) {
                        fprintf(output, "--> build took %lgs, but expected %s %lgs\n", test->result.buildtime / 1e6, getConstraintStr(constr->kind), constr->value / 1e6);
                    } else if (verbose) {
                        fprintf(output, "==> build took %lgs\n", test->result.buildtime / 1e6);
                    }
                    if (test->result.failed_build) {
                        if (test->result.buildexit != 0) {
                            fprintf(output, "--> build failed with non zero exit code %i\n", test->result.buildexit);
                        } else {
                            fprintf(output, "--> build terminated with signal %s\n", strsignal(test->result.buildsignal));
                        }
                    } else if (verbose) {
                        fprintf(output, "==> build exited with code %i\n", test->result.buildexit);
                    }
                }
            }
            if (strlen(test->config.run_command) != 0) {
                Constraint* constr = testAllIntConstraints(&test->config.time, test->result.runtime);
                if (test->result.out_of_runtime) {
                    fprintf(output, "--> run killed after %lgs, but expected %s %lgs\n", test->result.runtime / 1e6, getConstraintStr(constr->kind), constr->value / 1e6);
                } else {
                    if (constr != NULL) {
                        fprintf(output, "--> run took %lgs, but expected %s %lgs\n", test->result.runtime / 1e6, getConstraintStr(constr->kind), constr->value / 1e6);
                    } else if (verbose) {
                        fprintf(output, "==> run took %lgs\n", test->result.runtime / 1e6);
                    }
                    if (test->result.signal != 0) {
                        fprintf(output, "--> run terminated with signal %i\n", test->result.signal);
                    } else {
                        constr = testAllIntConstraints(&test->config.exit, test->result.exit);
                        if (constr != NULL) {
                            fprintf(output, "--> run exited with code %i, but expected %s %li\n", test->result.exit, getConstraintStr(constr->kind), constr->value);
                        } else if (verbose) {
                            fprintf(output, "==> run exited with code %i\n", test->result.exit);
                        }
                    }
                    constr = testAllStringConstraints(&test->config.err, test->result.err);
                    if (constr != NULL) {
                        fprintf(output, "--> stderr is '%s', but expected %s '%s'\n", test->result.err, getConstraintStr(constr->kind), constr->string);
                    } else if (verbose) {
                        fprintf(output, "==> stderr is '%s'\n", test->result.err);
                    }
                    constr = testAllStringConstraints(&test->config.out, test->result.out);
                    if (constr != NULL) {
                        fprintf(output, "--> stdout is '%s', but expected %s '%s'\n", test->result.out, getConstraintStr(constr->kind), constr->string);
                    } else if (verbose) {
                        fprintf(output, "==> stdout is '%s'\n", test->result.out);
                    }
                }
            }
            if (strlen(test->config.cleanup_command) != 0) {
                if (test->result.failed_cleanup) {
                    if (test->result.buildexit != 0) {
                        fprintf(output, "--> cleanup failed with non zero exit code %i\n", test->result.cleanupexit);
                    } else {
                        fprintf(output, "--> cleanup terminated with signal %s\n", strsignal(test->result.cleanupsignal));
                    }
                } else if (verbose) {
                    fprintf(output, "==> cleanup exited with code %i\n", test->result.cleanupexit);
                }
            }
        }
        fputc('\n', output);
    }
}

void printTestResults(TestList* tests, FILE* output, bool all, bool verbose) {
    printTestSummary(tests, output);
    for (int i = 0; i < tests->count; i++) {
        if (all || (tests->tests[i].result.completed && tests->tests[i].result.failed)) {
            printTestResult(&tests->tests[i], output, verbose);
        }
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
