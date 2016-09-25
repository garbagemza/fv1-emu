#include "MathUtils.h"
#include <math.h>

#ifdef _MSC_VER
#if _MSC_VER < 1800
double log2(double acc) {
	return log(acc) / log(2.0);
}

bool signbit(double n) {
	return n < 0.0;
}
#endif
#endif
double fv1log2(double acc) {
	if (acc < 0.00001526)
		acc = 0.00001526;

	if (acc > 0.99999988)
		acc = 0.99999988;

	return log2(acc);
}

double fv1exp2(double value) {
	return pow(2.0, value);
}