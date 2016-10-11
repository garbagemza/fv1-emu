#include <Windows.h>
#include "MemoryManager.h"
#include "..\Core\types.h"
#include <exception>

#define MAX_DELAY_SIZE 32768
double* MemoryManager::mem_ptr = NULL;
u32 MemoryManager::mem_alloc_ptr = 0;

using namespace std;

void MemoryManager::createMemoryPool() {
	mem_ptr = new double[MAX_DELAY_SIZE];
	if (mem_ptr != NULL) {
		memset(mem_ptr, 0, MAX_DELAY_SIZE * sizeof(double));
		mem_alloc_ptr = 0;
	}
}

Memory* MemoryManager::createMemory(unsigned int size) {
	if (mem_alloc_ptr + size > MAX_DELAY_SIZE) {
		throw exception("delay ram exeeded limits.");
	}

	double* ptr = &mem_ptr[mem_alloc_ptr];
	if (ptr != NULL) {
		mem_alloc_ptr += size;

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
	i32 index = mem->mem->tail_index + mem->displacement + mem->lfoDisplacement;
	index = index >= 0 ? index % mem->mem->size : mem->mem->size + index;
	return mem->mem->start_ptr[index];
}

double MemoryManager::getValueAtStart(MemoryAddress* mem) {
	unsigned int index = mem->mem->head_index + mem->displacement + mem->lfoDisplacement;
	index = index % mem->mem->size;
	return mem->mem->start_ptr[index];
}

double MemoryManager::getValueAtMiddle(MemoryAddress* mem) {
	unsigned int start_pointer = mem->mem->head_index;
	unsigned int mem_size = mem->mem->size;
	unsigned int mid_point = mem_size / 2; //(middle point: (0 + size) / 2)
	unsigned int index = start_pointer + mid_point + mem->displacement + mem->lfoDisplacement;
	index = index % mem_size;
	return mem->mem->start_ptr[index];
}

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

double MemoryManager::getValueAtAbsolutePosition(u32 position) {
	if (mem_ptr != NULL) {
		return mem_ptr[position];
	}
	return 0.0;
}
