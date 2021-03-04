#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "constraint.h"

typedef struct {
    char* name;
    // instructions
    int run_count;
    char* build_command;
    char* run_command;
    char* cleanup_command;
    char* stdin;
    // constraints
    Constraint buildtime;
    Constraint runtime;
    Constraint time;
    Constraint cputime;
    Constraint memory;
    Constraint exit;
    Constraint stdout;
    Constraint stderr;
} TestSettings;



#endif