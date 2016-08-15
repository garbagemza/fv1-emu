#pragma once
#include <string>
using namespace std;

#define FV1_SAMPLE_RATE 32768

struct Memory;
struct MemoryAddress;
struct Timer;

// flags used for CHO opcode
struct CHOFlags {

	static const unsigned int UNKNOWN_CHO_FLAG = -1;

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

	enum LFOType {
		SIN0,
		SIN1,
		RMP0,
		RMP1
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
	double pacc = 0;
	double acc = 0;
	double lr = 0;
	double adcl = 0;		// left input
	double adcr = 0;		// right input

	double dacl = 0;		// left output
	double dacr = 0;		// right output

	double pot0 = 0;
	double pot1 = 0;
	double pot2 = 0;

	double reg0 = 0;
	double reg1 = 0;
	double reg2 = 0;
	double reg3 = 0;
	double reg4 = 0;
	double reg5 = 0;
	double reg6 = 0;
	double reg7 = 0;
	double reg8 = 0;
	double reg9 = 0;
	double reg10 = 0;

	double reg11 = 0;
	double reg12 = 0;
	double reg13 = 0;
	double reg14 = 0;
	double reg15 = 0;
	double reg16 = 0;
	double reg17 = 0;
	double reg18 = 0;
	double reg19 = 0;

	double reg20 = 0;
	double reg21 = 0;
	double reg22 = 0;
	double reg23 = 0;
	double reg24 = 0;
	double reg25 = 0;
	double reg26 = 0;
	double reg27 = 0;
	double reg28 = 0;
	double reg29 = 0;
	double reg30 = 0;
	double reg31 = 0;

	// oscillators
	double sin0_rate = 0.0;
	double sin1_rate = 0.0;

	double sin0_range = 0;
	double sin1_range = 0;

	void mem(Memory** addr, unsigned int size);
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
	void wlds(LFOType osc, double frequencyCoefficient, double amplitudeCoefficient);
	void cho_rda(Timer* timer, LFOType osc, unsigned int choFlags, MemoryAddress* memAddress);

	// used for skip conditions
	bool zrc();
	bool zro();
	bool gez();
	bool neg();

	// misc
	void updm(Memory* mem);
	int displacementWithLFO(Timer* timer, int flags, double rate, double amplitude);

	double* getAddressOfIdentifier(string id);
	SkipCondition conditionWithIdentifier(string id);
	LFOType oscillatorWithIdentifier(string id);
	double* getRegisterAddressWithName(string name);
	
};