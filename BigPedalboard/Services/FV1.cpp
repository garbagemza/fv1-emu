#include "stdafx.h"
#include "FV1.h"
#include "..\Manager\MemoryManager.h"
#include "..\Utilities\MathUtils.h"

void FV1::mem(Memory** memory, unsigned int size) {
	*memory = MemoryManager::createMemory(size);
}

//Read register value, multiply by coefficient and add to ACC.
void FV1::rdax(double regValue, double coefficient) {
	//Previous ACC value is simultaneously loaded into PACC
	pacc = acc;
	acc = acc + regValue * coefficient;
}

// since the memory is read from the end, we switch read algorithm
void FV1::rda(Memory* mem, MemoryPosition pos, double coefficient) {
	double tmp = 0;
	switch (pos) {
	case Start:
		tmp = MemoryManager::getValueAtEnd(mem);
		break;
	case Middle:
		//tmp = MemoryManager::getValueAtMiddle(mem);
		break;
	case End:
		tmp = MemoryManager::getValueAtStart(mem);
		break;
	}

	lr = tmp;	//read data from delay memory is simultaneously loaded into the LR register
	pacc = acc;	//Previous ACC value is simultaneously loaded into PACC

	acc = acc + tmp * coefficient;
}

//Write delay memory, multiply written value by the coefficient, add to LR register and load ACC with result.
void FV1::wrap(Memory* mem, double coefficient) {
	MemoryManager::setValueAtStart(mem, acc);
	pacc = acc;										//Previous ACC value is simultaneously loaded into PACC
	acc = acc * coefficient + lr;
}

//Write ACC to register, multiply ACC by coefficient
void FV1::wrax(double* value_addr, double coefficient) {
	*value_addr = acc;
	pacc = acc;				//Previous ACC value is simultaneously loaded into PACC
	acc = acc * coefficient;
}

//Write ACC to delay memory location and multiply ACC by coefficient
void FV1::wra(Memory* mem, double coefficient) {
	MemoryManager::setValueAtStart(mem, acc);
	pacc = acc;
	acc = acc * coefficient;
}

//Load the accumulator with the product of ACC and a register.
void FV1::mulx(double* value_addr) {
	pacc = acc;
	acc = acc * (*value_addr);
}

//Subtract register contents from ACC, multiply by coefficient, add register contents and load to ACC.
void FV1::rdfx(double* value_addr, double coefficient) {
	pacc = acc;
	double regValue = (*value_addr);
	acc = regValue + (acc - regValue) * coefficient;
}

//Take the LOG base 2 of the absolute value of the accumulator, divide result by 16, multiply the result by the coefficient, add a constant and load to ACC.
void FV1::fv1log(double coefficient, double constant) {
	pacc = acc;
	double logAcc = fv1log2(abs(acc)) / 16.0;
	acc = coefficient * logAcc + constant;
}

//Raise 2 to the power of the accumulator * 16, multiply by a coefficient and add to a constant, loading the result to ACC.
void FV1::fv1exp(double coefficient, double constant) {
	pacc = acc;
	double expAcc = fv1exp2(acc * 16.0);
	acc = coefficient * expAcc + constant;
}

void FV1::sof(double scale, double offset) {
	pacc = acc;
	acc = acc * scale + offset;
}

// increments indexes by one
// when reaching the end of the buffer, they start again in zero, making a circular buffer
void FV1::updm(Memory* mem) {
	MemoryManager::updateMemoryPointersIncrementingByOne(mem);
}

double* FV1::getAddressOfIdentifier(string id) {
	if (id.compare("adcl") == 0) {
		return &adcl;
	}
	else if (id.compare("adcr") == 0) {
		return &adcr;
	}
	else if (id.compare("dacl") == 0) {
		return &dacl;
	}
	else if (id.compare("dacr") == 0) {
		return &dacr;
	}
	else if (id.compare("pot0") == 0) {
		return &pot0;
	}
	else if (id.compare("pot1") == 0) {
		return &pot1;
	}
	else if (id.compare("pot2") == 0) {
		return &pot2;
	}
	else if (id.compare("reg0") == 0) {
		return &reg0;
	}
	else if (id.compare("reg1") == 0) {
		return &reg1;
	}
	else if (id.compare("reg2") == 0) {
		return &reg2;
	}
	else if (id.compare("reg3") == 0) {
		return &reg3;
	}
	else if (id.compare("reg4") == 0) {
		return &reg4;
	}
	else if (id.compare("reg5") == 0) {
		return &reg5;
	}
	else if (id.compare("reg6") == 0) {
		return &reg6;
	}
	else if (id.compare("reg7") == 0) {
		return &reg7;
	}
	else if (id.compare("reg8") == 0) {
		return &reg8;
	}
	else if (id.compare("reg9") == 0) {
		return &reg9;
	}
	else if (id.compare("reg10") == 0) {
		return &reg10;
	}
	else if (id.compare("reg11") == 0) {
		return &reg11;
	}
	else if (id.compare("reg12") == 0) {
		return &reg12;
	}
	else if (id.compare("reg13") == 0) {
		return &reg13;
	}
	else if (id.compare("reg14") == 0) {
		return &reg14;
	}
	else if (id.compare("reg15") == 0) {
		return &reg15;
	}
	else if (id.compare("reg16") == 0) {
		return &reg16;
	}
	else if (id.compare("reg17") == 0) {
		return &reg17;
	}
	else if (id.compare("reg18") == 0) {
		return &reg18;
	}
	else if (id.compare("reg19") == 0) {
		return &reg19;
	}
	else if (id.compare("reg20") == 0) {
		return &reg20;
	}
	else if (id.compare("reg21") == 0) {
		return &reg21;
	}
	else if (id.compare("reg22") == 0) {
		return &reg22;
	}
	else if (id.compare("reg23") == 0) {
		return &reg23;
	}
	else if (id.compare("reg24") == 0) {
		return &reg24;
	}
	else if (id.compare("reg25") == 0) {
		return &reg25;
	}
	else if (id.compare("reg26") == 0) {
		return &reg26;
	}
	else if (id.compare("reg27") == 0) {
		return &reg27;
	}
	else if (id.compare("reg28") == 0) {
		return &reg28;
	}
	else if (id.compare("reg29") == 0) {
		return &reg29;
	}
	else if (id.compare("reg30") == 0) {
		return &reg30;
	}
	else if (id.compare("reg31") == 0) {
		return &reg31;
	}

	return 0;
}
