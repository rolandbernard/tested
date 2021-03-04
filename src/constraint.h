#ifndef _CONSTRAINT_H_
#define _CONSTRAINT_H_

#include <stdbool.h>

typedef enum {
    CONSTRAIND_EQUAL,
    CONSTRAIND_UNEQUAL,
    CONSTRAIND_LESS,
    CONSTRAIND_LESS_EQUAL,
    CONSTRAIND_MORE,
    CONSTRAIND_MORE_EQUAL,
    CONSTRAIND_COUNT,
} ConstraintKind;

typedef struct {
    ConstraintKind kind;
    union {
        long value;
        char* string;
    };
} Constraint;

typedef struct {
    int count;
    Constraint constraints[CONSTRAIND_COUNT];
} ConstraintList;

bool includesConstraintKind(ConstraintList* list, ConstraintKind kind);

Constraint* getConstraint(ConstraintList* list, ConstraintKind kind);

void insertConstraint(ConstraintList* list, Constraint constraint);

Constraint* testAllIntConstraints(ConstraintList* list, int value);

int getIntConstraintMaximum(ConstraintList* list);

bool areIntConstraintsSatisfiable(ConstraintList* list);

Constraint* testAllStringConstraints(ConstraintList* list, const char* value);

void freeStringConstraints(ConstraintList* list);

#endif