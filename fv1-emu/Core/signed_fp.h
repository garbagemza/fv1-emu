#pragma once
#include "types.h"

template <size_t IntegerBitCount, size_t FractionalBitCount>
class signed_fp
{
	// TODO: put static
	const u32 signBit = (1 << (IntegerBitCount + FractionalBitCount));
	const u32 signMask = setSignMask(32 - (IntegerBitCount + FractionalBitCount + 1));
	const u32 factor = (1 << FractionalBitCount);

	static inline u32 setSignMask(int bitNum) {
		u32 x = 0;
		for (int i = 31; i >= bitNum; i--)
			x |= (1L << i);

		return x;
	}
	i32 data;

public:
	signed_fp()
	{
		*this = 0.0;
	}

	signed_fp(i32 packed) {
		// extend sign if the packed value is negative
		bool isNegative = (packed & signBit) != 0;
		if (isNegative) {
			packed |= signMask;
		}
		data = packed;
	}
	signed_fp(f64 d)
	{
		*this = d; // calls operator=
	}

	signed_fp& operator=(f64 d)
	{
		double min = -pow(2.0, (double)(IntegerBitCount));
		double max = pow(2.0, (double)(IntegerBitCount)) - pow(2.0, -(double)(FractionalBitCount));
		if (d < min) d = min;
		if (d > max) d = max;

		data = (i32)floor(d * factor + 0.5);
		return *this;
	}

	i32 raw_data() const
	{
		return data;
	}

	f64 doubleValue() {
		return (f64)data / (f64)factor;
	}
	// Other operators can be defined here
};