#include "stdafx.h"
#include "MathUtils.h"
#include <math.h>

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