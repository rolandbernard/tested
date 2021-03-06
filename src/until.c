
#include <stdlib.h>
#include <string.h>

#include "util.h"

char* copyString(const char* str) {
    int length = strlen(str);
    char* ret = (char*)malloc(length + 1);
    memcpy(ret, str, length + 1);
    return ret;
}
