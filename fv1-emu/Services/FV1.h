#pragma once
#include <string>
#include "..\Core\types.h"

using namespace std;

#define FV1_SAMPLE_RATE 32768

struct Memory;
struct MemoryAddress;
struct Timer;

// flags used for CHO opcode
struct CHOFlags {

	static const unsigned int UNKNOWN_CHO_FLAG = 0xFFFFFFFF;

	static const unsigned int SIN = 0;			// $0 Select SIN output(default) (Sine LFO only)
	static const unsigned int COS = 1;			// Select COS output(Sine LFO only)
	static const unsigned int REG = 1 << 1;		// Save the output of the LFO into an internal LFO register.
	static const unsigned int COMPC = 1 << 2;	// Complement the coefficient(1-coeff)
	static const unsigned int COMPA = 1 << 3;	// Complement the address offset from the LFO
	static const unsigned int RPTR2 = 1 << 4;	// Select the ramp + 1 / 2 pointer(Ramp LFO only)
	static const unsigned int NA = 1 << 5;		// Select xfade coefficient and do not add address offset
};

class FV1 {
public:
	enum SkipCondition {
		UNKNOWN,
		RUN,
		ZRC,
		ZRO,
		GEZ,
		NEG
	};

	enum Register {
		ADCL,
		ADCR,
		DACL,
		DACR,

		POT0,
		POT1,
		POT2,

		REG0,
		REG1,
		REG2,
		REG3,
		REG4,
		REG5,
		REG6,
		REG7,
		REG8,
		REG9,
		REG10,

		REG11,
		REG12,
		REG13,
		REG14,
		REG15,
		REG16,
		REG17,
		REG18,
		REG19,

		REG20,
		REG21,
		REG22,
		REG23,
		REG24,
		REG25,
		REG26,
		REG27,
		REG28,
		REG29,

		REG30,
		REG31,

	};

	const static u32 SIN0 = 0;
	const static u32 SIN1 = 1;
	const static u32 RMP0 = 2;
	const static u32 RMP1 = 3;
	

	double pacc;
	double acc;
	double lr;
	double adcl;		// left input
	double adcr;		// right input

	double dacl;		// left output
	double dacr;		// right output

	double pot0;
	double pot1;
	double pot2;

	double reg0;
	double reg1;
	double reg2;
	double reg3;
	double reg4;
	double reg5;
	double reg6;
	double reg7;
	double reg8;
	double reg9;
	double reg10;

	double reg11;
	double reg12;
	double reg13;
	double reg14;
	double reg15;
	double reg16;
	double reg17;
	double reg18;
	double reg19;

	double reg20;
	double reg21;
	double reg22;
	double reg23;
	double reg24;
	double reg25;
	double reg26;
	double reg27;
	double reg28;
	double reg29;
	double reg30;
	double reg31;

	// oscillators
	double osc_reg = 0.0; // internal register
	double sin0_rate;
	double sin1_rate;
	double rmp0_rate;
	double rmp1_rate;

	double sin0_range;
	double sin1_range;
	u32 rmp0_range;
	u32 rmp1_range;

	double addr_ptr = 0;
	FV1() {
		pacc = 0;
		acc = 0;
		lr = 0;
		adcl = 0;
		adcr = 0;
		dacl = 0;
		dacr = 0;
		pot0 = 0;
		pot1 = 0;
		pot2 = 0;
		reg0 = 0;
		reg1 = 0;
		reg2 = 0;
		reg3 = 0;
		reg4 = 0;
		reg5 = 0;
		reg6 = 0;
		reg7 = 0;
		reg8 = 0;
		reg9 = 0;
		reg10 = 0;

		reg11 = 0;
		reg12 = 0;
		reg13 = 0;
		reg14 = 0;
		reg15 = 0;
		reg16 = 0;
		reg17 = 0;
		reg18 = 0;
		reg19 = 0;

		reg20 = 0;
		reg21 = 0;
		reg22 = 0;
		reg23 = 0;
		reg24 = 0;
		reg25 = 0;
		reg26 = 0;
		reg27 = 0;
		reg28 = 0;
		reg29 = 0;
		reg30 = 0;
		reg31 = 0;
		sin0_rate = 0.0;
		sin1_rate = 0.0;
		sin0_range = 0;
		sin1_range = 0;
		rmp0_rate = 0.0;
		rmp1_rate = 0.0;
		rmp0_range = 0;
		rmp1_range = 0;
	}

	void mem(Memory** addr, unsigned int size);
	void rmpa(u32 instruction);
	void rdax(double regValue, double coefficient);
	void rda(MemoryAddress* mem, double coefficient);
	void wrap(Memory* mem, double coefficient);
	void wrax(double* value_addr, double coefficient);
	void wra(Memory* mem, double coefficient);
	void mulx(double* value_addr);
	void rdfx(double* value_addr, double coefficient);
	void sof(double scale, double offset);
	void fv1log(double coefficient, double constant);
	void fv1exp(double coefficient, double constant);
	void maxx(double* reg_addr, double coefficient);
	void ldax(double* reg_addr);
	void absa();
	void wrlx(double* reg_addr, double coefficient);
	void wrhx(double* reg_addr, double coefficient);
	void wlds(u32 osc, double frequencyCoefficient, double amplitudeCoefficient);
	void wldr(u32 instruction);
	void cho(Timer* timer, MemoryAddress* memAddress, u32 instruction);
	void rdal(double oscValue);

	void or(unsigned int value);
	void and(unsigned int value);
	void xor(unsigned int value);

	// used for skip conditions
	bool zrc();
	bool zro();
	bool gez();
	bool neg();

	// misc
	void updm(Memory* mem);
	double xfadeCoefficientWithRange(u32 sample, u32 range);
	double xfadeCoefficientWithRange2(u32 sample, u32 range);
	double xfadeCoefficientWithRange3(u32 sample, u32 range);
	double* getAddressOfIdentifier(string id);
	SkipCondition conditionWithIdentifier(string id);
	u32 oscillatorWithIdentifier(string id);
	double* getRegisterAddressWithName(string name);
	
};