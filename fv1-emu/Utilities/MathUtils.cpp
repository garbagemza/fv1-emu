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

//S.23 Fixed point number with MSB as the sign bit, everything else is mantissa
// giving values between -1.0 and 1.0.
double hex2s23(unsigned int value) {
	double result = 0.0;
	bool sign = (value & 0x800000) != 0;
	long long mantissa = (value & 0x7FFFFF);
	result = sign ? -mantissa / 8388607.0 : mantissa / 8388607.0;
	return result;
}

unsigned int s23tohex(double value) {
	bool sign = value < 0.0;
	if (value > 1.0) value = 1.0;
	if (value < -1.0) value = -1.0;

	unsigned int result = sign ? 1 << 23 : 0;
	value = sign ? -value : value;
	unsigned int mantissa = (unsigned int)floor((value * 8388607.0) + 0.5);
	result |= mantissa;
	return result;
}