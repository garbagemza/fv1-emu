#include "stdafx.h"
#include "FV1.h"
#include "..\Manager\MemoryManager.h"
#include "..\Manager\TimerManager.h"
#include "..\Utilities\MathUtils.h"
#include "..\Utilities\SoundUtilities.h"

#include <cassert>

#define _USE_MATH_DEFINES
#include <math.h>


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
void FV1::rda(MemoryAddress* mem, double coefficient) {
	double tmp = 0;
	switch (mem->position) {
	case Start:
		tmp = MemoryManager::getValueAtEnd(mem);
		break;
	case Middle:
		tmp = MemoryManager::getValueAtMiddle(mem);
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

void FV1::maxx(double* reg_addr, double coefficient) {
	pacc = acc;
	double absReg = abs((*reg_addr) * coefficient);
	acc = max(absReg, abs(acc));
}

void FV1::ldax(double* reg_addr) {
	pacc = acc;
	acc = (*reg_addr);
}

void FV1::absa() {
	pacc = acc;
	acc = abs(acc);
}

void FV1::wrlx(double* reg_addr, double coefficient) {
	(*reg_addr) = acc;
	double tmpAcc = (pacc - acc) * coefficient + pacc;
	pacc = acc;
	acc = tmpAcc;
}

void FV1::wrhx(double* reg_addr, double coefficient) {
	(*reg_addr) = acc;
	double tmpAcc = (acc * coefficient) + pacc;
	pacc = acc;
	acc = tmpAcc;
}

void FV1::wlds(LFOType osc, unsigned int Kf, unsigned int Ka) {
	switch (osc) {
	case SIN0:
		sin0_rate = (double)Kf * (double)FV1_SAMPLE_RATE / (131072.0 * 2.0 * M_PI);
		sin0_range = (unsigned int)(floor((double)Ka * 16385.0 / 32767.0));

		break;
	case SIN1:
		sin1_rate = (double)Kf * (double)FV1_SAMPLE_RATE / (131072.0 * 2.0 * M_PI);
		sin1_range = (unsigned int)(floor((double)Ka * 16385.0 / 32767.0));
		break;
	default:
		assert(false); // invalid LFO type
	}
}

// for now the coefficient will be 0.5, in the original chip the 
// interpolation coefficient is taken from the calculation of the
// sine/cosine.
void FV1::cho_rda(Timer* timer, LFOType osc, int choFlags, MemoryAddress* memAddress) {

	double rate = 0.0;
	unsigned int amplitude = 0;

	if (osc == SIN0) {
		rate = sin0_rate;
		amplitude = sin0_range / 2;
	}
	else if (osc == SIN1) {
		rate = sin1_rate;
		amplitude = sin1_range / 2;
	}
	else if (osc == RMP0) {
		assert(false);// not implemented yet
	}
	else if (osc == RMP1) {
		assert(false);// not implemented yet
	}

	memAddress->lfoDisplacement = displacementWithLFO(timer, choFlags, rate, amplitude);

	rda(memAddress, 0.5);
}

bool FV1::zrc() {
	return signbit(pacc) != signbit(acc);
}

bool FV1::zro() {
	return acc == 0.0;
}

bool FV1::gez() {
	return acc >= 0.0;
}

bool FV1::neg() {
	return signbit(acc);
}


// increments indexes by one
// when reaching the end of the buffer, they start again in zero, making a circular buffer
void FV1::updm(Memory* mem) {
	MemoryManager::updateMemoryPointersIncrementingByOne(mem);
}


double* FV1::getAddressOfIdentifier(string id) {
	if (_stricmp(id.c_str(), "adcl") == 0) {
		return &adcl;
	}
	else if (_stricmp(id.c_str(), "adcr") == 0) {
		return &adcr;
	}
	else if (_stricmp(id.c_str(), "dacl") == 0) {
		return &dacl;
	}
	else if (_stricmp(id.c_str(), "dacr") == 0) {
		return &dacr;
	}
	else if (_stricmp(id.c_str(), "pot0") == 0) {
		return &pot0;
	}
	else if (_stricmp(id.c_str(), "pot1") == 0) {
		return &pot1;
	}
	else if (_stricmp(id.c_str(), "pot2") == 0) {
		return &pot2;
	}
	else if (_stricmp(id.c_str(), "reg0") == 0) {
		return &reg0;
	}
	else if (_stricmp(id.c_str(), "reg1") == 0) {
		return &reg1;
	}
	else if (_stricmp(id.c_str(), "reg2") == 0) {
		return &reg2;
	}
	else if (_stricmp(id.c_str(), "reg3") == 0) {
		return &reg3;
	}
	else if (_stricmp(id.c_str(), "reg4") == 0) {
		return &reg4;
	}
	else if (_stricmp(id.c_str(), "reg5") == 0) {
		return &reg5;
	}
	else if (_stricmp(id.c_str(), "reg6") == 0) {
		return &reg6;
	}
	else if (_stricmp(id.c_str(), "reg7") == 0) {
		return &reg7;
	}
	else if (_stricmp(id.c_str(), "reg8") == 0) {
		return &reg8;
	}
	else if (_stricmp(id.c_str(), "reg9") == 0) {
		return &reg9;
	}
	else if (_stricmp(id.c_str(), "reg10") == 0) {
		return &reg10;
	}
	else if (_stricmp(id.c_str(), "reg11") == 0) {
		return &reg11;
	}
	else if (_stricmp(id.c_str(), "reg12") == 0) {
		return &reg12;
	}
	else if (_stricmp(id.c_str(), "reg13") == 0) {
		return &reg13;
	}
	else if (_stricmp(id.c_str(), "reg14") == 0) {
		return &reg14;
	}
	else if (_stricmp(id.c_str(), "reg15") == 0) {
		return &reg15;
	}
	else if (_stricmp(id.c_str(), "reg16") == 0) {
		return &reg16;
	}
	else if (_stricmp(id.c_str(), "reg17") == 0) {
		return &reg17;
	}
	else if (_stricmp(id.c_str(), "reg18") == 0) {
		return &reg18;
	}
	else if (_stricmp(id.c_str(), "reg19") == 0) {
		return &reg19;
	}
	else if (_stricmp(id.c_str(), "reg20") == 0) {
		return &reg20;
	}
	else if (_stricmp(id.c_str(), "reg21") == 0) {
		return &reg21;
	}
	else if (_stricmp(id.c_str(), "reg22") == 0) {
		return &reg22;
	}
	else if (_stricmp(id.c_str(), "reg23") == 0) {
		return &reg23;
	}
	else if (_stricmp(id.c_str(), "reg24") == 0) {
		return &reg24;
	}
	else if (_stricmp(id.c_str(), "reg25") == 0) {
		return &reg25;
	}
	else if (_stricmp(id.c_str(), "reg26") == 0) {
		return &reg26;
	}
	else if (_stricmp(id.c_str(), "reg27") == 0) {
		return &reg27;
	}
	else if (_stricmp(id.c_str(), "reg28") == 0) {
		return &reg28;
	}
	else if (_stricmp(id.c_str(), "reg29") == 0) {
		return &reg29;
	}
	else if (_stricmp(id.c_str(), "reg30") == 0) {
		return &reg30;
	}
	else if (_stricmp(id.c_str(), "reg31") == 0) {
		return &reg31;
	}

	return 0;
}

FV1::SkipCondition FV1::conditionWithIdentifier(string id) {
	if (_stricmp(id.c_str(), "run") == 0) {
		return FV1::RUN;
	}
	else if (_stricmp(id.c_str(), "zrc") == 0) {
		return FV1::ZRC;
	}
	else if (_stricmp(id.c_str(), "zro") == 0) {
		return FV1::ZRO;
	}
	else if (_stricmp(id.c_str(), "gez") == 0) {
		return FV1::GEZ;
	}
	else if (_stricmp(id.c_str(), "neg") == 0) {
		return FV1::NEG;
	}

	return FV1::UNKNOWN;
}

FV1::LFOType FV1::oscillatorWithIdentifier(string id) {
	const char* str = id.c_str();
	if (_stricmp(str, "sin0") == 0) {
		return FV1::SIN0;
	}
	else if (_stricmp(str, "sin1") == 0) {
		return FV1::SIN1;
	}
	else if (_stricmp(str, "rmp0") == 0) {
		return FV1::RMP0;
	}
	else if (_stricmp(str, "rmp1") == 0) {
		return FV1::RMP1;
	}

	// default
	return FV1::SIN0;
}

// use sin oscillator as default, this changes if COS is included in flags.
int FV1::displacementWithLFO(Timer* timer, int choFlags, double rate, unsigned int amplitude) {
	bool useCosineInsteadSin = false;
	bool saveToRegister = false;

	if (choFlags & CHOFlags::COS) {
		useCosineInsteadSin = true;
	}

	if (choFlags & CHOFlags::REG) {
		saveToRegister = true;
	}

	double w = 2.0 * M_PI * rate;
	//double period = 1.0 / rate;

	double sincos = useCosineInsteadSin ? cos(w * timer->t) : sin(w * timer->t);
	int displacement = (int)(floor(sincos * (double)amplitude));
	return displacement;
}