#pragma once
#ifdef OVERFLOW
	#undef OVERFLOW
#endif

#ifdef UNDERFLOW
	#undef UNDERFLOW
#endif

enum STR2NUMBER_ERROR { 
	SUCCESS, 
	OVERFLOW, 
	UNDERFLOW, 
	INCONVERTIBLE 
};

STR2NUMBER_ERROR str2uint(unsigned int &i, char const *s, int base = 0);
STR2NUMBER_ERROR str2int(int &i, char const *s, int base = 0);
STR2NUMBER_ERROR str2dble(double& d, char const *s, int base = 0);
STR2NUMBER_ERROR hexstr2uint(unsigned int& value, const char *s);
STR2NUMBER_ERROR binstr2uint(unsigned int& value, const char *s);
