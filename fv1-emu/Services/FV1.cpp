#include "FV1.h"
#include "..\Manager\MemoryManager.h"
#include "..\Manager\TimerManager.h"
#include "..\Utilities\MathUtils.h"
#include "..\Utilities\SoundUtilities.h"
#include "..\Core\signed_fp.h"
#include <cassert>

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

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

void FV1::wlds(u32 osc, double Kf, double Ka) {
	switch (osc) {
	case SIN0:
		sin0_rate = Kf * (double)FV1_SAMPLE_RATE / (131072.0 * 2.0 * M_PI);
		sin0_range = floor(Ka * 16385.0 / 32767.0);

		break;
	case SIN1:
		sin1_rate = Kf * (double)FV1_SAMPLE_RATE / (131072.0 * 2.0 * M_PI);
		sin1_range = floor(Ka * 16385.0 / 32767.0);
		break;
	default:
		assert(false); // invalid LFO type
	}
}

// 160 ms(t*640000 iterations)
// *25ms (isolated)
void FV1::wldr(u32 instruction) {
	u32 amp = (instruction & 0x60) >> 5;
	u32 ampValue = 512 << amp; 

	i16 freq = (instruction & 0x1FFFE000) >> 13;
	u32 osc = (instruction & 0x20000000) >> 29;

	switch (osc) {
	case 0:
		rmp0_rate = (double)freq / 32768.0;
		rmp0_range = ampValue;
		break;
	case 1:
		rmp1_rate = (double)freq / 32768.0;
		rmp1_range = ampValue;
		break;
	default:
		assert(false); // invalid LFO type
	}
	return;
}


void FV1::cho(Timer* timer, MemoryAddress* memAddress, u32 instruction) {

	u32 osc = (instruction & 0x600000) >> 21;
	u32 flags = (instruction & 0x3F000000) >> 24;
	i32 constant = (instruction & 0x1FFFE0) >> 5;
	u32 subOperation = (instruction & 0xC0000000) >> 30;
	i32 displacement = 0;
	u32 range = 0;

	bool isRampOscillator = osc == RMP0 || osc == RMP1;
	bool isSinOscillator = osc == SIN0 || osc == SIN1;

	bool cosine = (flags & 0x01) != 0;
	bool reg = (flags & 0x02) != 0;
	bool compc = (flags & 0x04) != 0;
	bool compa = (flags & 0x08) != 0;
	bool rptr2 = (flags & 0x10) != 0;
	bool na = (flags & 0x20) != 0;
	double coefficient = 0.5;
	double rate = 0.0;

	assert(!compa); // not implemented yet

	if (isRampOscillator) {
		range = osc == RMP0 ? rmp0_range : rmp1_range;
		rate = osc == RMP0 ? rmp0_rate : rmp1_rate;
	}
	if (isSinOscillator) {
		rate = osc == SIN0 ? sin0_rate : sin1_rate;
		range = osc == SIN0 ? static_cast<u32>(sin0_range / 2.0) : static_cast<u32>(sin1_range / 2.0);
	}

	double w = 2.0 * M_PI * rate;

	if (reg) {
		if (isSinOscillator) {
			osc_reg = cosine ? cos(w * timer->t) : sin(w * timer->t);
		}
		else if (isRampOscillator) {
			double x = (double)(timer->sample % range) / (double)range;
			osc_reg = x * rate;
			
		}
	}

	if (rptr2) {
		double xrptr2 = (double)((timer->sample + range / 2) % range) / (double)range;
		osc_reg = xrptr2 * rate;
	}

	if (na) {
		coefficient = xfadeCoefficientWithRange(timer->sample, range);
		assert(coefficient >= 0.0 && coefficient <= 1.0); // invalid coefficient
	}
	else {
		double displacementDouble = osc_reg * (double)range;
		displacement = static_cast<i32>(displacementDouble);
		coefficient = displacementDouble - (double)displacement;
	}

	if (compc) {
		coefficient = 1.0 - coefficient;
	}


	switch (subOperation)
	{
	case 0: // rda
		{
			memAddress->lfoDisplacement = displacement;
			rda(memAddress, coefficient);
		}
		break;
	case 2: // sof
		{
			double offset = signed_fp<0, 15>(constant).doubleValue();
			sof(coefficient, offset);
		}
		break;
	case 3: // rdal
		assert(false); // not implemented yet
		break;
	default:
		assert(false); // not implemented yet
		break;
	}
}

void FV1::or(unsigned int value) {
	pacc = acc;
	unsigned int hexAcc = s23tohex(acc);
	hexAcc |= value;
	acc = hex2s23(hexAcc);
}

void FV1::and(unsigned int value) {
	pacc = acc;
	unsigned int hexAcc = s23tohex(acc);
	hexAcc &= value;
	acc = hex2s23(hexAcc);

}

void FV1::xor(unsigned int value) {
	pacc = acc;
	unsigned int hexAcc = s23tohex(acc);
	hexAcc ^= value;
	acc = hex2s23(hexAcc);
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
	const char * str = id.c_str();

	if (_stricmp(str, "adcl") == 0) {
		return &adcl;
	}
	else if (_stricmp(str, "adcr") == 0) {
		return &adcr;
	}
	else if (_stricmp(str, "dacl") == 0) {
		return &dacl;
	}
	else if (_stricmp(str, "dacr") == 0) {
		return &dacr;
	}
	else if (_stricmp(str, "pot0") == 0) {
		return &pot0;
	}
	else if (_stricmp(str, "pot1") == 0) {
		return &pot1;
	}
	else if (_stricmp(str, "pot2") == 0) {
		return &pot2;
	}
	else if (_stricmp(str, "sin0_rate") == 0) {
		return &sin0_rate;
	}
	else if (_stricmp(str, "sin0_range") == 0) {
		return &sin0_range;
	}
	else if (_stricmp(str, "sin1_rate") == 0) {
		return &sin1_rate;
	}
	else if (_stricmp(str, "sin1_range") == 0) {
		return &sin1_range;
	}
	else if (_stricmp(str, "rmp0_rate") == 0) {
		return &rmp0_rate;
	}
	else if (_stricmp(str, "rmp1_rate") == 0) {
		return &rmp1_rate;
	}
	else if (_stricmp(str, "reg0") == 0) {
		return &reg0;
	}
	else if (_stricmp(str, "reg1") == 0) {
		return &reg1;
	}
	else if (_stricmp(str, "reg2") == 0) {
		return &reg2;
	}
	else if (_stricmp(str, "reg3") == 0) {
		return &reg3;
	}
	else if (_stricmp(str, "reg4") == 0) {
		return &reg4;
	}
	else if (_stricmp(str, "reg5") == 0) {
		return &reg5;
	}
	else if (_stricmp(str, "reg6") == 0) {
		return &reg6;
	}
	else if (_stricmp(str, "reg7") == 0) {
		return &reg7;
	}
	else if (_stricmp(str, "reg8") == 0) {
		return &reg8;
	}
	else if (_stricmp(str, "reg9") == 0) {
		return &reg9;
	}
	else if (_stricmp(str, "reg10") == 0) {
		return &reg10;
	}
	else if (_stricmp(str, "reg11") == 0) {
		return &reg11;
	}
	else if (_stricmp(str, "reg12") == 0) {
		return &reg12;
	}
	else if (_stricmp(str, "reg13") == 0) {
		return &reg13;
	}
	else if (_stricmp(str, "reg14") == 0) {
		return &reg14;
	}
	else if (_stricmp(str, "reg15") == 0) {
		return &reg15;
	}
	else if (_stricmp(str, "reg16") == 0) {
		return &reg16;
	}
	else if (_stricmp(str, "reg17") == 0) {
		return &reg17;
	}
	else if (_stricmp(str, "reg18") == 0) {
		return &reg18;
	}
	else if (_stricmp(str, "reg19") == 0) {
		return &reg19;
	}
	else if (_stricmp(str, "reg20") == 0) {
		return &reg20;
	}
	else if (_stricmp(str, "reg21") == 0) {
		return &reg21;
	}
	else if (_stricmp(str, "reg22") == 0) {
		return &reg22;
	}
	else if (_stricmp(str, "reg23") == 0) {
		return &reg23;
	}
	else if (_stricmp(str, "reg24") == 0) {
		return &reg24;
	}
	else if (_stricmp(str, "reg25") == 0) {
		return &reg25;
	}
	else if (_stricmp(str, "reg26") == 0) {
		return &reg26;
	}
	else if (_stricmp(str, "reg27") == 0) {
		return &reg27;
	}
	else if (_stricmp(str, "reg28") == 0) {
		return &reg28;
	}
	else if (_stricmp(str, "reg29") == 0) {
		return &reg29;
	}
	else if (_stricmp(str, "reg30") == 0) {
		return &reg30;
	}
	else if (_stricmp(str, "reg31") == 0) {
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

u32 FV1::oscillatorWithIdentifier(string id) {
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

double FV1::xfadeCoefficientWithRange(u32 sampleNumber, u32 range) {

	u32 x = sampleNumber % range; // get number between 0 and range
	u32 quarter = range >> 2;
	u32 octave = range >> 3;
	double m = 1.0 / (double)quarter;
	if (x >= 0 && x < octave) {
		return 0.0;
	}
	else if (x >= octave && x < (octave + quarter)) {
		return m*(x - octave);
	}
	else if (x >= (octave + quarter) && x < (octave + 2 * quarter)) {
		return 1.0;
	}
	else if (x >= (octave + 2 * quarter) && x < (octave + 3 * quarter)) {
		return -m*(x - (2*quarter + octave)) + 1.0;
	}
	else {
		return 0.0;
	}
}

double FV1::xfadeCoefficientWithRange2(u32 sampleNumber, u32 range) {
	i32 x = sampleNumber % range; // get number between 0 and range
	i32 quarter = range >> 2;
	i32 octave = range >> 3;
	double m = 1.0 / (double)octave;
	if (x >= 0 && x < octave) {
		return 0.0;
	}
	else if (x >= octave && x < quarter) {
		return m*(x - octave);
	}
	else if (x >= quarter && x < 3 * quarter) {
		return 1.0;
	}
	else if (x >= 3 * quarter && x < (octave + 3 * quarter)) {
		return -m * (double)(x - (octave + 3 * quarter));
	}
	else {
		return 0.0;
	}
	
}

double FV1::xfadeCoefficientWithRange3(u32 sampleNumber, u32 range) {
	i32 x = sampleNumber % range; // get number between 0 and range
	i32 quarter = range >> 2;
	i32 octave = range >> 3;
	i32 sixtieth = range >> 4;

	double m = 1.0 / (double)sixtieth;
	if (x >= 0 && x < octave) {
		return 0.0;
	}
	else if (x >= octave && x < (octave + sixtieth)) {
		return m*(x - (octave + sixtieth));
	}
	else if (x >= (octave + sixtieth) && x < (sixtieth + 3 * quarter)) {
		return 1.0;
	}
	else if (x >= (sixtieth + 3 * quarter) && x < (octave + 3 * quarter)) {
		return -m * (double)(x - (octave + 3 * quarter));
	}
	else {
		return 0.0;
	}

}
