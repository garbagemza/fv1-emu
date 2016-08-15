#pragma once
#include "Lexer.h"
#include "FV1.h"

enum MemoryPosition;


struct Param
{
	Lexer::TOKEN_TYPE type;
	string value;
	double doubleValue;

	double* regAddress;
	MemoryAddress* memAddress;

	// used for skp instruction
	FV1::SkipCondition condition;
	FV1::LFOType osc;
	unsigned int unsignedIntValue;

	Param() {
		doubleValue = 0;
		regAddress = 0;
		memAddress = 0;
		condition = FV1::UNKNOWN;
		osc = FV1::SIN0;
		unsignedIntValue = 0;
	}
};

enum Opcode {
	UNKNOWN,
	RDAX,
	RDA,
	WRAP,
	WRAX,
	WRA,
	MULX,
	RDFX,
	LOG,
	EXP,
	SOF,
	MAXX,
	LDAX,
	SKP,
	ABSA,
	WRLX,
	WRHX,
	WLDS,
	CHO,		// only used by the parser, the interpreter does not recognize this opcode
	CHO_RDA,

};


struct Instruction
{
	Opcode opcode;
	Param* args[5];
};

struct ExecutionVectorResult
{
	bool success;
	vector<vector<Lexer::Token*>> firstPass;
	vector<vector<Lexer::Token*>> secondPass;

	vector<string> firstPassRawLines;
	vector<string> secondPassRawLines;
};

struct PassOneResult {
	bool success;
	map<string, Memory*> memoryMap;
	map<string, Param> equatesMap;
};

struct PassTwoResult {
	bool success;
	unsigned int instructionCount;
	Instruction* instructions[128];
};

struct SpinFile {

	map<string, Param> equMap;
	map<string, Memory*> memMap;

	PassTwoResult passTwo;
	bool passOneSemanticSucceeded;

	SpinFile() {
		passOneSemanticSucceeded = false;
	}
};

struct SplitStatementInfo
{
	bool shouldSplit;
	string labelDeclared;
	vector<Lexer::Token*> secondPassStatement;
};

class Parser {

	ExecutionVectorResult	generateExecutionVector(vector<vector<Lexer::Token*>>, vector<string>);
	BOOL					isFirstPassStatement(vector<Lexer::Token*>);
	SplitStatementInfo		shouldSplitStatements(vector<Lexer::Token*>);
	BOOL					LoadInstructionWithInstructionLine(vector<Lexer::Token*>, unsigned int, Instruction*);
	vector<Param*>			GetParameters(vector<Lexer::Token*> line, unsigned int);
	Param*					GetParameter(vector<Lexer::Token*>& line, unsigned int);

	Opcode					InstructionOpcodeWithString(string&);
	MemoryPosition			DirectionSpecificationWithType(Lexer::TOKEN_TYPE&);
	Opcode					opcodeWithSecondaryOpcode(Opcode opcode, string subOpcode);
	unsigned int			getChoFlagWithString(string id);
	unsigned int			getChoFlagsValueWithLine(vector<Lexer::Token*>& line);

	BOOL					isChoFlag(string id);
	BOOL					isHexValue(string id);

	FV1* fv1;

public:
	Parser(FV1* fv1);
	ExecutionVectorResult	beginLexicalAnalysis(LPVOID lpBuffer, DWORD size);
	BOOL					PassOneParse(vector<vector<Lexer::Token*>>);
	PassTwoResult			PassTwoParse(vector<vector<Lexer::Token*>>, vector<string>);

	map<string, Param> equMap;
	map<string, Memory*> memMap;
	map<string, unsigned int> labels;

};
