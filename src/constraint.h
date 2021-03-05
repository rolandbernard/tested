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

void insertIntConstraint(ConstraintList* list, Constraint constraint);

void insertStringConstraint(ConstraintList* list, Constraint constraint);

Constraint* testAllIntConstraints(ConstraintList* list, long value);

long getIntConstraintMaximum(ConstraintList* list);

bool areIntConstraintsSatisfiable(ConstraintList* list);

Constraint* testAllStringConstraints(ConstraintList* list, const char* value);

void copyStringConstraints(ConstraintList* dst, ConstraintList* src);

void deinitStringConstraints(ConstraintList* list);

#endif