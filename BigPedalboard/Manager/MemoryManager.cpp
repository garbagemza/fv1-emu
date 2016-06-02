#include "stdafx.h"
#include "MemoryManager.h"

Memory* MemoryManager::createMemory(unsigned int size) {

	double* ptr = new double[size];
	if (ptr != NULL) {
		memset(ptr, 0, size * sizeof(double));
		Memory* mem = new Memory();
		if (mem != NULL) {
			mem->head_index = 0;
			mem->tail_index = size - 1;
			mem->size = size;
			mem->start_ptr = ptr;
			return mem;
		}
	}
	return NULL;
}

double MemoryManager::getValueAtEnd(MemoryAddress* mem) {
	unsigned int index = mem->mem->tail_index + mem->displacement;
	index = index % mem->mem->size;
	return mem->mem->start_ptr[index];
}

double MemoryManager::getValueAtStart(MemoryAddress* mem) {
	unsigned int index = mem->mem->head_index + mem->displacement;
	index = index % mem->mem->size;
	return mem->mem->start_ptr[index];
}

// if the size is odd, returns the middle point of the buffer
// if the size is even, returns integer value of middle point
//double MemoryManager::getValueAtMiddle(Memory* mem) {
//	unsigned int head = mem->head_index;
//	unsigned int tail = mem->tail_index;
//	if (tail > head) {
//		unsigned int middle = (tail + head) / 2;
//		return middle;
//	}
//	else if (tail < head) {
//
//	}
//}

void MemoryManager::setValueAtStart(Memory* mem, double value) {
	mem->start_ptr[mem->head_index] = value;
}

void MemoryManager::updateMemoryPointersIncrementingByOne(Memory* mem) {
	unsigned int current_head = mem->head_index;
	unsigned int current_tail = mem->tail_index;

	unsigned int new_tail = current_head;
	unsigned int new_head = current_head;

	if (current_head + 1 == mem->size) {
		new_head = 0;
	}
	else {
		new_head = current_head + 1;
	}

	mem->tail_index = new_tail;
	mem->head_index = new_head;
}