#pragma once

struct Memory {
	double * start_ptr;
	unsigned int size;
	unsigned int head_index;
	unsigned int tail_index;
};

class MemoryManager {
public:
	static Memory* createMemory(unsigned int size);
	static double getValueAtEnd(Memory* mem);
	static double getValueAtStart(Memory* mem);
	//static double getValueAtMiddle(Memory* mem);
	static void setValueAtStart(Memory* mem, double value);
	static void updateMemoryPointersIncrementingByOne(Memory* mem);

};