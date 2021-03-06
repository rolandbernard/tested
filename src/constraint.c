
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "constraint.h"

#include "util.h"

bool includesConstraintKind(ConstraintList* list, ConstraintKind kind) {
    if (list->count == CONSTRAINT_COUNT) {
        return true;
    } else {
        for (int i = 0; i < list->count; i++) {
            if (list->constraints[i].kind == kind) {
                return true;
            }
        }
        return false;
    }
}

Constraint* getConstraint(ConstraintList* list, ConstraintKind kind) {
    for (int i = 0; i < list->count; i++) {
        if (list->constraints[i].kind == kind) {
            return &list->constraints[i];
        }
    }
    return NULL;
}

void insertIntConstraint(ConstraintList* list, Constraint constraint) {
    Constraint* existing = getConstraint(list, constraint.kind);
    if (existing == NULL) {
        if (constraint.kind == CONSTRAINT_UNEQUAL) {
            existing = getConstraint(list, CONSTRAINT_EQUAL);
        } else if (constraint.kind == CONSTRAINT_EQUAL) {
            existing = getConstraint(list, CONSTRAINT_UNEQUAL);
        }
    }
    if (existing != NULL) {
        existing->kind = constraint.kind;
        existing->value = constraint.value;
    } else {
        list->constraints[list->count] = constraint;
        list->count++;
    }
}

void insertStringConstraint(ConstraintList* list, Constraint constraint) {
    Constraint* existing = getConstraint(list, constraint.kind);
    if (existing == NULL) {
        if (constraint.kind == CONSTRAINT_UNEQUAL) {
            existing = getConstraint(list, CONSTRAINT_EQUAL);
        } else if (constraint.kind == CONSTRAINT_EQUAL) {
            existing = getConstraint(list, CONSTRAINT_UNEQUAL);
        }
    }
    if (existing != NULL) {
        free(existing->string);
        existing->kind = constraint.kind;
        existing->string = constraint.string;
    } else {
        list->constraints[list->count] = constraint;
        list->count++;
    }
}

Constraint* testAllIntConstraints(ConstraintList* list, long value) {
    for (int i = 0; i < list->count; i++) {
        bool satisfied = true;
        switch (list->constraints[i].kind) {
        case CONSTRAINT_EQUAL:
            satisfied = (value == list->constraints[i].value);
            break;
        case CONSTRAINT_UNEQUAL:
            satisfied = (value != list->constraints[i].value);
            break;
        case CONSTRAINT_LESS:
            satisfied = (value < list->constraints[i].value);
            break;
        case CONSTRAINT_LESS_EQUAL:
            satisfied = (value <= list->constraints[i].value);
            break;
        case CONSTRAINT_MORE:
            satisfied = (value > list->constraints[i].value);
            break;
        case CONSTRAINT_MORE_EQUAL:
            satisfied = (value >= list->constraints[i].value);
            break;
        default: break;
        }
        if (!satisfied) {
            return &list->constraints[i];
        }
    }
    return NULL;
}

long getIntConstraintMaximum(ConstraintList* list) {
    long max = LONG_MAX;
    for (int i = 0; i < list->count; i++) {
        switch (list->constraints[i].kind) {
        case CONSTRAINT_EQUAL:
        case CONSTRAINT_LESS_EQUAL:
            if (list->constraints[i].value < max) {
                max = list->constraints[i].value;
            }
            break;
        case CONSTRAINT_LESS:
            if (list->constraints[i].value - 1 < max) {
                max = list->constraints[i].value - 1;
            }
            break;
        default: break;
        }
    }
    return max;
}

bool areIntConstraintsSatisfiable(ConstraintList* list) {
    long min = LONG_MIN;
    long max = LONG_MAX;
    long cannot_be = 0;
    bool unequal = false;
    for (int i = 0; i < list->count; i++) {
        long value = list->constraints[i].value;
        switch (list->constraints[i].kind) {
        case CONSTRAINT_EQUAL:
            if (value > min) {
                min = value;
            }
            if (value < max) {
                max = value;
            }
            break;
        case CONSTRAINT_UNEQUAL:
            cannot_be = value;
            unequal = true;
            break;
        case CONSTRAINT_LESS:
            if (value - 1 < max) {
                max = value - 1;
            }
            break;
        case CONSTRAINT_LESS_EQUAL:
            if (value < max) {
                max = value;
            }
            break;
        case CONSTRAINT_MORE:
            if (value + 1 > min) {
                min = value + 1;
            }
            break;
        case CONSTRAINT_MORE_EQUAL:
            if (value > min) {
                min = value;
            }
            break;
        default: break;
        }
    }
    return max >= min && (!unequal || max != min || max != cannot_be);
}

bool areStringConstraintsSatisfiable(ConstraintList* list) {
    const char* min = NULL;
    const char* max = NULL;
    const char* cannot_be = 0;
    bool unequal = false;
    for (int i = 0; i < list->count; i++) {
        const char* value = list->constraints[i].string;
        switch (list->constraints[i].kind) {
        case CONSTRAINT_EQUAL:
            if (min == NULL || strcmp(value, min) > 0) {
                min = value;
            }
            if (max == NULL || strcmp(value, max) < 0) {
                max = value;
            }
            break;
        case CONSTRAINT_UNEQUAL:
            cannot_be = value;
            unequal = true;
            break;
        case CONSTRAINT_LESS:
        case CONSTRAINT_LESS_EQUAL:
            if (max == NULL || strcmp(value, max) < 0) {
                max = value;
            }
            break;
        case CONSTRAINT_MORE:
        case CONSTRAINT_MORE_EQUAL:
            if (min == NULL || strcmp(value, min) > 0) {
                min = value;
            }
            break;
        default: break;
        }
    }
    return min == NULL || max == NULL || (strcmp(max, min) >= 0 && (!unequal || strcmp(max, min) != 0 || strcmp(max, cannot_be) != 0));
}

Constraint* testAllStringConstraints(ConstraintList* list, const char* value) {
    for (int i = 0; i < list->count; i++) {
        bool satisfied = false;
        switch (list->constraints[i].kind) {
        case CONSTRAINT_EQUAL:
            satisfied = (strcmp(value, list->constraints[i].string) == 0);
            break;
        case CONSTRAINT_UNEQUAL:
            satisfied = (strcmp(value, list->constraints[i].string) != 0);
            break;
        case CONSTRAINT_LESS:
            satisfied = (strcmp(value, list->constraints[i].string) < 0);
            break;
        case CONSTRAINT_LESS_EQUAL:
            satisfied = (strcmp(value, list->constraints[i].string) <= 0);
            break;
        case CONSTRAINT_MORE:
            satisfied = (strcmp(value, list->constraints[i].string) > 0);
            break;
        case CONSTRAINT_MORE_EQUAL:
            satisfied = (strcmp(value, list->constraints[i].string) >= 0);
            break;
        default: break;
        }
        if (!satisfied) {
            return &list->constraints[i];
        }
    }
    return NULL;
}

void copyStringConstraints(ConstraintList* dst, ConstraintList* src) {
    dst->count = src->count;
    for (int i = 0; i < src->count; i++) {
        dst->constraints[i].kind = src->constraints[i].kind;
        dst->constraints[i].string = copyString(src->constraints[i].string);
    }
}

void deinitStringConstraints(ConstraintList* list) {
    for (int i = 0; i < list->count; i++) {
        free(list->constraints[i].string);
    }
}
