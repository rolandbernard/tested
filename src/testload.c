
#include <stdlib.h>
#include <stdio.h>  
#include <string.h>  
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
        return (char*)realloc(buffer, length + 1);
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

static int loadInteger(const char* line, const char** end) {
    int ret = 0;
    while (*line != 0) {
        if (*line >= '0' && *line <= '9') {
            ret *= 10;
            ret += *line - '0';
        }
        line++;
    }
    if (end != NULL) {
        *end = line;
    }
    return ret;
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

static void loadIntConstraints(ConstraintList* list, const char* line) {
    while (*line != 0) {
        Constraint constr;
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
        if (isspace(*line)) {
            line++;
        }
        constr.value = loadInteger(line, &line);
        insertConstraint(list, constr);
    }
}

static void loadStringConstraints(ConstraintList* list, const char* line) {
    Constraint constr;
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
    if (isspace(*line)) {
        line++;
    }
    constr.string = loadString(line);
    insertConstraint(list, constr);
}

static void loadLine(TestCaseSettings* config, const char* line) {
    line = skipSpace(line);
    if (strncmp(line, "test", 4) == 0) {
        line = skipToValue(line + 4);
        if (line != NULL) {
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
            config->run_count = loadInteger(line, NULL);
        }
    } else if (strncmp(line, "run", 3) == 0) {
        line = skipToValue(line + 3);
        if (line != NULL) {
            config->run_count = loadInteger(line, NULL);
        }
    } else if (strncmp(line, "cleanup", 7) == 0) {
        line = skipToValue(line + 7);
        if (line != NULL) {
            config->run_count = loadInteger(line, NULL);
        }
    } else if (strncmp(line, "stdin", 5) == 0) {
        line = skipToValue(line + 5);
        if (line != NULL) {
            config->run_count = loadInteger(line, NULL);
        }
    } else if (strncmp(line, "buildtime", 9) == 0) {
        line = skipToValue(line + 9);
        if (line != NULL) {
            loadIntConstraints(&config->buildtime, line);
        }
    } else if (strncmp(line, "buildcputime", 12) == 0) {
        line = skipToValue(line + 12);
        if (line != NULL) {
            loadIntConstraints(&config->buildcputime, line);
        }
    } else if (strncmp(line, "time", 4) == 0) {
        line = skipToValue(line + 4);
        if (line != NULL) {
            loadIntConstraints(&config->time, line);
        }
    } else if (strncmp(line, "cputime", 7) == 0) {
        line = skipToValue(line + 7);
        if (line != NULL) {
            loadIntConstraints(&config->cputime, line);
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
    }
}

static const char* comment_start[] = {
    "//",
    "#",
    ";",
    "Rem",
};

bool tryToLoadTest(TestCase* test, TestCaseSettings* def, FILE* file) {
    char* line = readLine(file);
    if (line != NULL) {
        char* actual_line = NULL;
        for (int i = 0; actual_line == NULL && i < sizeof(comment_start)/sizeof(comment_start[0]); i++) {
            if (strncmp(comment_start[i], line, strlen(comment_start[i])) == 0) {
                actual_line = line + strlen(comment_start[i]);
            }
        }
        if (actual_line != NULL) {
            while (isspace(actual_line[0])) {
                actual_line++;
            }
            if (strncmp(actual_line, "test", 4) == 0) {
                while (actual_line != NULL) {
                    loadLine(&test->settings, actual_line);
                    free(line);
                    line = readLine(file);
                    actual_line = NULL;
                    if (line != NULL) {
                        for (int i = 0; actual_line == NULL && i < sizeof(comment_start)/sizeof(comment_start[0]); i++) {
                            if (strncmp(comment_start[i], line, strlen(comment_start[i])) == 0) {
                                actual_line = line + strlen(comment_start[i]);
                            }
                        }
                    }
                }
                free(line);
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void loadConfig(TestCaseSettings* config, FILE* file) {
    char* line = readLine(file);
    while (line != NULL) {
        loadLine(config, line);
        free(line);
        line = readLine(file);
    }
}
