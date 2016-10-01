#pragma once
#include "types.h"

template <size_t IntegerBitCount, size_t FractionalBitCount>
class signed_fp
{
	// TODO: put static
	const unsigned int signMask = 1 << (IntegerBitCount + FractionalBitCount);
	const unsigned int factor = (1 << FractionalBitCount);

	i32 data;

public:
	signed_fp()
	{
		*this = 0.0;
	}

	signed_fp(f32 d)
	{
		*this = d; // calls operator=
	}

	signed_fp& operator=(f32 d)
	{
		//i32 min = -2 ^ (IntegerBitCount);
		//i32 max = 2 ^ (IntegerBitCount)- 2 ^ -(FractionalBitCount);
		//-1.0 and 1 − 2^−(N - 1), when N is number of bits
		i32 intPart = static_cast<i32>(d);

		data = (i32)floor(d * factor + 0.5);
		return *this;
	}

	i32 raw_data() const
	{
		return data;
	}

	f32 doubleValue() {
		return (f32)data / (f32)factor;
	}
	// Other operators can be defined here
};