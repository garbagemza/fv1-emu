#include "stdafx.h"

#include <cerrno>
#include <cmath>
#undef OVERFLOW
#undef UNDERFLOW

#include "ParseUtil.h"

STR2NUMBER_ERROR str2int(int &i, char const *s, int base)
{
	char *end;
	long  l;
	errno = 0;
	l = strtol(s, &end, base);
	if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) {
		return OVERFLOW;
	}
	if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) {
		return UNDERFLOW;
	}
	if (*s == '\0' || *end != '\0') {
		return INCONVERTIBLE;
	}
	i = l;
	return SUCCESS;
}

STR2NUMBER_ERROR str2uint(unsigned int &i, char const *s, int base) {
	char *end;
	unsigned long  l;
	errno = 0;
	l = strtoul(s, &end, base);
	if ((errno == ERANGE && l == ULONG_MAX) || l > UINT_MAX) {
		return OVERFLOW;
	}
	if (*s == '\0' || *end != '\0') {
		return INCONVERTIBLE;
	}
	i = l;
	return SUCCESS;
}

STR2NUMBER_ERROR str2dble(double& d, char const *s, int base) {
	char *end;
	
	errno = 0;
	double dbl = strtod(s, &end);
	if (errno == ERANGE  && dbl == HUGE_VAL) {
		return OVERFLOW;
	}
	if (errno == ERANGE && dbl == -HUGE_VAL) {
		return UNDERFLOW;
	}

	if (*s == '\0' || *end != '\0') {
		return INCONVERTIBLE;
	}

	d = dbl;
	return SUCCESS;
}