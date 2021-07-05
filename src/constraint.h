#ifndef _CONSTRAINT_H_
#define _CONSTRAINT_H_

#include <stdbool.h>

typedef enum {
    CONSTRAINT_EQUAL,
    CONSTRAINT_UNEQUAL,
    CONSTRAINT_LESS,
    CONSTRAINT_LESS_EQUAL,
    CONSTRAINT_MORE,
    CONSTRAINT_MORE_EQUAL,
    CONSTRAINT_COUNT,
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
    Constraint constraints[CONSTRAINT_COUNT];
} ConstraintList;

bool includesConstraintKind(ConstraintList* list, ConstraintKind kind);

Constraint* getConstraint(ConstraintList* list, ConstraintKind kind);

void insertIntConstraint(ConstraintList* list, Constraint constraint);

void insertStringConstraint(ConstraintList* list, Constraint constraint);

Constraint* testAllIntConstraints(ConstraintList* list, long value);

long getIntConstraintMaximum(ConstraintList* list);

bool areIntConstraintsSatisfiable(ConstraintList* list);

bool areStringConstraintsSatisfiable(ConstraintList* list);

Constraint* testAllStringConstraints(ConstraintList* list, const char* value);

void copyStringConstraints(ConstraintList* dst, ConstraintList* src);

void deinitStringConstraints(ConstraintList* list);

#endif
