#include "endianutils.h"

unsigned long NativeFunction(void *arg) {
    long x = (long) arg;
    x = x * x;
    return (unsigned long) x;
}
