
#include <stdlib.h>
#include <stdio.h>  
#include <string.h>  
#include <strings.h>  
#include <ctype.h>  

#include "testload.h"

#define INITIAL_LINE_BUFFER_SIZE 128

static char* readLine(FILE* file) {
    int capacity = INITIAL_LINE_BUFFER_SIZE;
    char* buffer = (char*)malloc(INITIAL_LINE_BUFFER_SIZE);
    int length = 0;
    int next_char = fgetc(file);
    while (next_char != EOF && next_char != '\n') {
        if (length == capacity) {
            capacity *= 2;
            buffer = (char*)realloc(buffer, capacity);
        }
        buffer[length] = next_char;
        length++;
        next_char = fgetc(file);
    }
    if (length == 0 && next_char == EOF) {
        free(buffer);
        return NULL;
    } else {
        buffer = (char*)realloc(buffer, length + 1);
        buffer[length] = 0;
        return buffer;
    }
}

static const char* skipSpace(const char* line) {
    while (isspace(*line)) {
        line++;
    }
    return line;
}

static const char* skipToValue(const char* line) {
    line = skipSpace(line);
    if (*line == ':') {
        line++;
        if (isspace(*line)) {
            line++;
        }
        return line;
    } else {
        return NULL;
    }
}

static long loadInteger(const char* line, const char** end) {
    long ret = 0;
    while (*line >= '0' && *line <= '9') {
        ret *= 10;
        ret += *line - '0';
        line++;
    }
    if (end != NULL) {
        *end = line;
    }
    return ret;
}

static long loadTime(const char* line, const char** end) {
    const char* num_end;
    int value = loadInteger(line, &num_end);
    num_end = skipSpace(num_end);
    if (*num_end == 's' || *num_end == 'S') {
        value *= 1000000;
        num_end++;
    } else {
        if (*num_end == 'm' || *num_end == 'M') {
            value *= 1000;
            num_end++;
        } else if (*num_end == 'u' || *num_end == 'U') {
            num_end++;
        }
        if (*num_end == 's' || *num_end == 'S') {
            num_end++;
        }
    }
    *end = num_end;
    return value;
}

static char* loadString(const char* line) {
    char* ret = (char*)malloc(strlen(line) + 1);
    int len = 0;
    for (int i = 0; line[i] != 0; i++) {
        if (line[i] == '\\') {
            switch (line[i + 1]) {
            case 'n':
                ret[len] = '\n';
                break;
            default:
                if (line[i + 1] != 0) {
                    ret[len] = line[i + 1];
                }
                break;
            }
            i++;
            len++;
        } else {
            ret[len] = line[i];
            len++;
        }
    }
    ret[len] = 0;
    return ret;
}

static bool loadBoolean(const char* line) {
    if (strncasecmp(line, "true", 4) == 0 || strncasecmp(line, "yes", 3) == 0 || strncasecmp(line, "on", 2) == 0) {
        return true;
    } else {
        return false;
    }
}

static void loadIntConstraints(ConstraintList* list, const char* line) {
    while (*line != 0) {
        line = skipSpace(line);
        Constraint constr = { .kind = CONSTRAIND_EQUAL };
        if (line[0] == '=') {
            line++;
            if (line[0] == '=') {
                line++;
            }
            constr.kind = CONSTRAIND_EQUAL;
        } else if (line[0] == '!' && line[1] == '=') {
            line += 2;
            constr.kind = CONSTRAIND_UNEQUAL;
        } else if (line[0] == '>' && line[1] == '=') {
            line += 2;
            constr.kind = CONSTRAIND_MORE_EQUAL;
        } else if (line[0] == '>') {
            line++;
            constr.kind = CONSTRAIND_MORE;
        } else if (line[0] == '<' && line[1] == '=') {
            line += 2;
            constr.kind = CONSTRAIND_LESS_EQUAL;
        } else if (line[0] == '<') {
            line++;
            constr.kind = CONSTRAIND_LESS;
        }
        line = skipSpace(line);
        if (*line >= '0' && *line <= '9') {
            constr.value = loadInteger(line, &line);
            insertIntConstraint(list, constr);
        } else {
            break;
        }
    }
}

static void loadTimeConstraints(ConstraintList* list, const char* line) {
    while (*line != 0) {
        line = skipSpace(line);
        Constraint constr = { .kind = CONSTRAIND_EQUAL };
        if (line[0] == '=') {
            line++;
            if (line[0] == '=') {
                line++;
            }
            constr.kind = CONSTRAIND_EQUAL;
        } else if (line[0] == '!' && line[1] == '=') {
            line += 2;
            constr.kind = CONSTRAIND_UNEQUAL;
        } else if (line[0] == '>' && line[1] == '=') {
            line += 2;
            constr.kind = CONSTRAIND_MORE_EQUAL;
        } else if (line[0] == '>') {
            line++;
            constr.kind = CONSTRAIND_MORE;
        } else if (line[0] == '<' && line[1] == '=') {
            line += 2;
            constr.kind = CONSTRAIND_LESS_EQUAL;
        } else if (line[0] == '<') {
            line++;
            constr.kind = CONSTRAIND_LESS;
        }
        line = skipSpace(line);
        if (*line >= '0' && *line <= '9') {
            constr.value = loadTime(line, &line);
            insertIntConstraint(list, constr);
        } else {
            break;
        }
    }
}

static void loadStringConstraints(ConstraintList* list, const char* line) {
    Constraint constr = { .kind = CONSTRAIND_COUNT };
    if (line[0] == '=') {
        line++;
        if (line[0] == '=') {
            line++;
        }
        constr.kind = CONSTRAIND_EQUAL;
    } else if (line[0] == '!' && line[1] == '=') {
        line += 2;
        constr.kind = CONSTRAIND_UNEQUAL;
    } else if (line[0] == '>' && line[1] == '=') {
        line += 2;
        constr.kind = CONSTRAIND_MORE_EQUAL;
    } else if (line[0] == '>') {
        line++;
        constr.kind = CONSTRAIND_MORE;
    } else if (line[0] == '<' && line[1] == '=') {
        line += 2;
        constr.kind = CONSTRAIND_LESS_EQUAL;
    } else if (line[0] == '<') {
        line++;
        constr.kind = CONSTRAIND_LESS;
    }
    if (constr.kind == CONSTRAIND_COUNT) {
        constr.kind = CONSTRAIND_EQUAL;
    } else {
        if (isspace(*line)) {
            line++;
        }
    }
    constr.string = loadString(line);
    insertStringConstraint(list, constr);
}

static void loadLine(TestCaseConfig* config, const char* line) {
    line = skipSpace(line);
    if (strncmp(line, "test", 4) == 0) {
        line = skipToValue(line + 4);
        if (line != NULL) {
            free(config->name);
            config->name = loadString(line);
        }
    } else if (strncmp(line, "runs", 4) == 0) {
        line = skipToValue(line + 4);
        if (line != NULL) {
            config->run_count = loadInteger(line, NULL);
        }
    } else if (strncmp(line, "build", 5) == 0) {
        line = skipToValue(line + 5);
        if (line != NULL) {
            free(config->build_command);
            config->build_command = loadString(line);
        }
    } else if (strncmp(line, "run", 3) == 0) {
        line = skipToValue(line + 3);
        if (line != NULL) {
            free(config->run_command);
            config->run_command = loadString(line);
        }
    } else if (strncmp(line, "cleanup", 7) == 0) {
        line = skipToValue(line + 7);
        if (line != NULL) {
            free(config->cleanup_command);
            config->cleanup_command = loadString(line);
        }
    } else if (strncmp(line, "stdin", 5) == 0) {
        line = skipToValue(line + 5);
        if (line != NULL) {
            free(config->in);
            config->in = loadString(line);
        }
    } else if (strncmp(line, "buildtime", 9) == 0) {
        line = skipToValue(line + 9);
        if (line != NULL) {
            loadTimeConstraints(&config->buildtime, line);
        }
    } else if (strncmp(line, "time", 4) == 0) {
        line = skipToValue(line + 4);
        if (line != NULL) {
            loadTimeConstraints(&config->time, line);
        }
    } else if (strncmp(line, "exit", 4) == 0) {
        line = skipToValue(line + 4);
        if (line != NULL) {
            loadIntConstraints(&config->exit, line);
        }
    } else if (strncmp(line, "stderr", 6) == 0) {
        line = skipToValue(line + 6);
        if (line != NULL) {
            loadStringConstraints(&config->err, line);
        }
    } else if (strncmp(line, "stdout", 6) == 0) {
        line = skipToValue(line + 6);
        if (line != NULL) {
            loadStringConstraints(&config->out, line);
        }
    } else if (strncmp(line, "timesout", 8) == 0) {
        line = skipToValue(line + 8);
        if (line != NULL) {
            config->times_out = loadBoolean(line);
        }
    } else if (strncmp(line, "buildtimesout", 13) == 0) {
        line = skipToValue(line + 13);
        if (line != NULL) {
            config->times_out_build = loadBoolean(line);
        }
    }
}

static const char* comment_start[] = {
    "//",
    "#",
    ";",
    "REM",
};

bool tryToLoadTest(TestCase* test, TestCaseConfig* def, FILE* file) {
    char* line = readLine(file);
    if (line != NULL) {
        char* actual_line = NULL;
        for (int i = 0; actual_line == NULL && i < (int)(sizeof(comment_start)/sizeof(comment_start[0])); i++) {
            if (strncasecmp(comment_start[i], line, strlen(comment_start[i])) == 0) {
                actual_line = line + strlen(comment_start[i]);
            }
        }
        if (actual_line != NULL) {
            while (isspace(actual_line[0])) {
                actual_line++;
            }
            if (strncmp(actual_line, "test", 4) == 0) {
                initTestResult(&test->result);
                copyTestConfig(&test->config, def);
                while (actual_line != NULL) {
                    loadLine(&test->config, actual_line);
                    free(line);
                    line = readLine(file);
                    actual_line = NULL;
                    if (line != NULL) {
                        for (int i = 0; actual_line == NULL && i < (int)(sizeof(comment_start)/sizeof(comment_start[0])); i++) {
                            if (strncmp(comment_start[i], line, strlen(comment_start[i])) == 0) {
                                actual_line = line + strlen(comment_start[i]);
                            }
                        }
                    }
                }
                free(line);
                return true;
            } else {
                free(line);
                return false;
            }
        } else {
            free(line);
            return false;
        }
    } else {
        return false;
    }
}

void loadConfig(TestCaseConfig* config, FILE* file) {
    char* line = readLine(file);
    while (line != NULL) {
        loadLine(config, line);
        free(line);
        line = readLine(file);
    }
}
