#pragma once

double fv1log2(double);
double fv1exp2(double);
#ifdef _MSC_VER 
#if _MSC_VER < 1800
bool signbit(double);
#endif
#endif
