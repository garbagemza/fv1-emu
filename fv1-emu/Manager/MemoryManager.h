#pragma once
#include "..\Core\types.h"

struct Memory {
	double * start_ptr;
	unsigned int size;
	unsigned int head_index;
	unsigned int tail_index;
};

enum MemoryPosition {
	Start,
	Middle,
	End
};

struct MemoryAddress {
	Memory* mem;
	signed int displacement;
	signed int lfoDisplacement;
	MemoryPosition position;

	MemoryAddress() {
		mem = 0;
		displacement = 0;
		lfoDisplacement = 0;
		position = Start;
	}
};

class MemoryManager {
public:
	static void createMemoryPool();
	static Memory* createMemory(unsigned int size);
	static double getValueAtEnd(MemoryAddress* mem);
	static double getValueAtStart(MemoryAddress* mem);
	static double getValueAtMiddle(MemoryAddress* mem);
	static void setValueAtStart(Memory* mem, double value);
	static void updateMemoryPointersIncrementingByOne(Memory* mem);

private:
	static double* mem_ptr;
	static u32 mem_alloc_ptr;
};