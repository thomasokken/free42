#include <math.h>

extern "C" double log2(double x) {
    return log(x) / log(2.0);
}
