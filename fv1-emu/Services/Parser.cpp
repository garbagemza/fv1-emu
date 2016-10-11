#include "Parser.h"
#include "..\Utilities\ParseUtil.h"
#include "..\Manager\MemoryManager.h"
#include "..\Utilities\SCLog.h"
#include "..\Utilities\MathUtils.h"

#include <cassert>
#include <Windows.h>
#include "..\Core\types.h"
#include "..\Core\signed_fp.h"

Parser::Parser(FV1* fv1) {
	this->fv1 = fv1;
}

ExecutionVectorResult Parser::beginLexicalAnalysis(LPVOID lpBuffer, DWORD size) {
	Lexer lex = Lexer(lpBuffer, size);
	vector<vector<Lexer::Token*>> lines;
	vector<string> linesStr;

	while (lex.hasNextLine()) {
		string lineStr;
		try {
			lineStr = lex.getNextLine();
			if (lineStr.size() > 0) {
				SCLogInfo(L"Tokenizing line: %s", lineStr.c_str());
			}

			vector<Lexer::Token*> line = lex.getTokenizedNextLine();
			if (line.size() > 0) {
				lines.push_back(line);
				linesStr.push_back(lineStr);
			}
		}
		catch (std::exception& e) {
			SCLogError(L"Unable to tokenize: %s, exception: %s", lineStr.c_str(), e.what());
			ExecutionVectorResult err = ExecutionVectorResult();
			err.success = false;
			return err;
		}
	}

	if (lines.size() > 0) {
		// ok, the lexical analysis has been successful
		return generateExecutionVector(lines, linesStr);
	}

	ExecutionVectorResult err = ExecutionVectorResult();
	err.success = false;
	return err;
}

// first pass is to identify memory declarations and equates
// initializes memory and equates dictionaries
BOOL Parser::PassOneParse(vector<vector<Lexer::Token*>> lines) {
	BOOL success = false;

	for (vector<vector<Lexer::Token*>>::iterator it = lines.begin(); it != lines.end(); it++) {
		vector<Lexer::Token*> v = (*it);
		if (v.size() >= 3) {
			Lexer::TOKEN_TYPE type = v[0]->type;
			if (type == Lexer::TOKEN_TYPE::IDENTIFIER) {
				string id = v[0]->name;
				if (id.compare("mem") == 0) {
					if (v[1]->type == Lexer::TOKEN_TYPE::IDENTIFIER
						&& v[2]->type == Lexer::TOKEN_TYPE::NUMBER) {
						unsigned int sizeUInt = 0;
						STR2NUMBER_ERROR err = str2uint(sizeUInt, v[2]->name.c_str());
						if (err == SUCCESS) {
							Memory* memory = MemoryManager::createMemory(sizeUInt);
							if (memory != NULL) {
								memMap[v[1]->name] = memory;

							}
						}
					}
				}
				else if (id.compare("equ") == 0) {
					if (v[1]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
						Param equ = Param();
						equ.type = v[2]->type;
						equ.value = v[2]->name;

						if (v[2]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
							equ.regAddress = fv1->getAddressOfIdentifier(v[2]->name);

							map<string, Memory*>::iterator it = memMap.find(equ.value);
							if (it != memMap.end()) {
								vector<Lexer::Token*> line = v;
								line.erase(line.begin());
								line.erase(line.begin());
								line.erase(line.begin());
								MemoryAddress* memAddress = parseMemoryAddressWithLine(line, Start, (*it).second);
								equ.memAddress = memAddress;
							}

						}
						else if (v[2]->type == Lexer::TOKEN_TYPE::NUMBER) {
							if (isHexS23Value(equ.value)) {
								unsigned int hexValue = 0;
								string value = equ.value.replace(0, 1, "0x");
								STR2NUMBER_ERROR err = hexstr2uint(hexValue, value.c_str());
								if (err == SUCCESS) {

									// check mathutils
									equ.doubleValue = hex2s23(hexValue);
									equ.unsignedIntValue = hexValue;
								}
							}
							else {
								double coeff = 0;
								STR2NUMBER_ERROR err = str2dble(coeff, v[2]->name.c_str());
								if (err == SUCCESS) {
									equ.doubleValue = coeff;
								}
							}
						}
						equMap[v[1]->name] = equ;
					}
				}
			}
		}

	}
	success = true;
	return success;
}


PassTwoResult Parser::PassTwoParse(vector<vector<Lexer::Token*>> lines, vector<string> linesStr) {
	PassTwoResult result = PassTwoResult();
	result.success = false;
	unsigned int index = 0;
	for (vector<vector<Lexer::Token*>>::iterator it = lines.begin(); it != lines.end(); it++) {
		vector<Lexer::Token*> v = (*it);
		Instruction* inst = new Instruction();
		string lineStr = linesStr[index];
		SCLogInfo(L"Parsing line: %s", lineStr.c_str());
		BOOL instructionLoaded = LoadInstructionWithInstructionLine(v, index, inst);
		if (instructionLoaded) {
			result.instructions[index] = inst;
		}
		else {
			SCLogError(L"Unable to parse line %s", lineStr.c_str());
			return result;
		}
		index++;
	}

	result.instructionCount = index;
	result.success = true;
	return result;
}

BOOL Parser::LoadInstructionWithInstructionLine(vector<Lexer::Token*> line, unsigned int currentInstruction, Instruction* instruction) {
	if (line.size() > 0) {
		Lexer::TOKEN_TYPE type = line[0]->type;
		if (type == Lexer::TOKEN_TYPE::IDENTIFIER) {
			string inst = line[0]->name;
			Opcode opcode = InstructionOpcodeWithString(inst);
			if (opcode == WLDR) {
				line.erase(line.begin() + 0);
				return loadWLDRWithInstructionLine(line, currentInstruction, instruction);
			}
			else if (opcode == CHO) {
				line.erase(line.begin() + 0);
				instruction->opcode = opcode;
				return loadCHOWithInstructionLine(line, currentInstruction, instruction);
			}
			else if (opcode == RMPA) {
				line.erase(line.begin() + 0);
				instruction->opcode = opcode;
				return loadRMPAWithInstructionLine(line, currentInstruction, instruction);
			}
			else if (opcode != UNKNOWN) {
				line.erase(line.begin() + 0); // remove first element
				instruction->opcode = opcode;
				vector<Param*> params = GetParameters(line, currentInstruction);

				if (params.size() > 0) {
					int index = 0;
					for (vector<Param*>::iterator it = params.begin(); it != params.end(); it++) {
						Param* param = (*it);
						SCLogInfo(L">> Parameter: %s", param->value.c_str());
						instruction->args[index++] = param;
					}
				}
				SCLogInfo(L"");
				return true;
			}
		}
	}
	return false;
}

BOOL Parser::loadWLDRWithInstructionLine(vector<Lexer::Token*> line, unsigned int currentInstructionNumber, Instruction* instruction) {

	u32 inst = WLDR;
	instruction->opcode = WLDR;
	Param* lfo = GetParameter(line, currentInstructionNumber);
	if (lfo != NULL) {
		u32 rampLFONumber = lfo->unsignedIntValue;
		if (rampLFONumber >= 2) {
			if (_stricmp(lfo->value.c_str(), "rmp0") == 0) {
				rampLFONumber = 0;
			}
			else if (_stricmp(lfo->value.c_str(), "rmp1") == 0) {
				rampLFONumber = 1;
			}
		}
		if (rampLFONumber < 2) {
			inst |= (rampLFONumber << 29);

			Param* frequencyParam = GetParameter(line, currentInstructionNumber);
			if (frequencyParam != NULL) {
				i32 freq = static_cast<i32>(frequencyParam->doubleValue);
				if (freq >= -16384 && freq <= 32767) {
					freq &= 0xFFFF;
					inst |= (freq << 13);

					Param* amplitudeParam = GetParameter(line, currentInstructionNumber);
					if (amplitudeParam != NULL) {

						u32 amp = static_cast<u32>(amplitudeParam->doubleValue);
						u32 amplitudeBits = 0; // 512 is default
						switch (amp) {
						case 512: amplitudeBits = 0; break;
						case 1024: amplitudeBits = 1; break;
						case 2048: amplitudeBits = 2; break;
						case 4096: amplitudeBits = 3; break;
						default:
							return false; // invalid value, accepted (512, 1024, 2048, 4096);
						}

						inst |= (amplitudeBits << 5);
						instruction->rawValue = inst;
						return true;
					}
					return false; // amplitude parameter expected
				}
				return false; // invalid range, accepted (-16384 .. 32767)
			}
			return false; // frequency parameter expected
		}
		return false; // invalid value, accepted 0..1
	}
	return false;
}

BOOL Parser::loadCHOWithInstructionLine(vector<Lexer::Token*> line, unsigned int currentInstructionNumber, Instruction* instruction) {
	u32 inst = CHO;

	Param* secondaryOpcode = GetParameter(line, currentInstructionNumber);
	if (secondaryOpcode != NULL) {
		Opcode secondOpcode = opcodeWithSecondaryOpcode(CHO, secondaryOpcode->value);
		u32 choSubcode = 0;
		switch (secondOpcode)
		{
		case CHO_RDA: choSubcode = 0x00; break;
		case CHO_SOF: choSubcode = 0x02; break;
		case CHO_RDAL: choSubcode = 0x03; break;
		default: assert(false); // unrecognized subcode
			break;
		}
		inst |= (choSubcode << 30);
		Param* lfoParam = GetParameter(line, currentInstructionNumber);
		if (lfoParam != NULL) {
			u32 lfo = lfoParam->unsignedIntValue;
			if (lfo < 4) {
				inst |= (lfo << 21);
				Param* choFlags = GetParameter(line, currentInstructionNumber);
				if (choFlags != NULL) {
					u32 flags = choFlags->unsignedIntValue;
					if (flags <= 0x3f) {
						inst |= (flags << 24);
						Param* coefficientParam = GetParameter(line, currentInstructionNumber);
						if (coefficientParam != NULL) {
							if (coefficientParam->memAddress != NULL) {
								instruction->memAddress = coefficientParam->memAddress;
							}
							else {
								signed_fp<0, 15> s15constant(coefficientParam->doubleValue);
								double test = s15constant.doubleValue();
								i32 packeds15 = s15constant.raw_data();
								inst |= ((packeds15 & 0xFFFF) << 5);
							}
							instruction->rawValue = inst;
							return true;
						}
						return false; // invalid param, coefficient expected
					}
					return false; // invalid range, expected cho flags
				}
				return false; // invalid param, expected cho flags
			}
			return false; // invalid param, expected (sin0, sin1, rmp0, rmp1)
		}
		return false; // expected parameter (lfotype)
	}
	return false; // subcode expected
}

BOOL Parser::loadRMPAWithInstructionLine(vector<Lexer::Token*> line, unsigned int currentInstructionNumber, Instruction* instruction) {
	u32 inst = RMPA;
	instruction->opcode = RMPA;
	inst |= 0x18 << 5;	// value convention

	Param* coefficientParam = GetParameter(line, currentInstructionNumber);
	if (coefficientParam != NULL) {
		signed_fp<1, 9> packedCoefficient(coefficientParam->doubleValue);
		double test = packedCoefficient.doubleValue();
		i32 coeff = packedCoefficient.raw_data();
		inst |= ((coeff & 0x7FF) << 21);

		instruction->rawValue = inst;
		return true;
	}

	return false;
}

vector<Param*> Parser::GetParameters(vector<Lexer::Token*> line, unsigned int currentInstruction) {
	vector<Param*> parameters;
	Param* param = GetParameter(line, currentInstruction);
	if (param != NULL) {
		parameters.push_back(param);
		vector<Param*> params = GetParameters(line, currentInstruction);
		if (params.size() > 0) {
			parameters.insert(parameters.end(), params.begin(), params.end());
		}
	}	
	return parameters;
}

Param* Parser::GetParameter(vector<Lexer::Token*>& line, unsigned int currentInstruction) {
	Param* arg0 = new Param();
	if (line.size() > 0) {
		MemoryPosition position = Start;
		bool negative = false;
		
		// first is optional
		if (line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
			negative = true;
			line.erase(line.begin());
		}

		if (line[0]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
			arg0->type = line[0]->type;
			arg0->value = line[0]->name;

			line.erase(line.begin());

			if (line.size() > 0) {
				// next token is optional, it can be numeral (#) or circumflex (^)
				// if followed by a comma, the direction specification is the default.
				if (line[0]->type == Lexer::TOKEN_TYPE::NUMERAL || line[0]->type == Lexer::TOKEN_TYPE::CIRCUMFLEX) {
					position = DirectionSpecificationWithType(line[0]->type);
					line.erase(line.begin());
				}
			}

			arg0->regAddress = fv1->getAddressOfIdentifier(arg0->value);
			if (arg0->regAddress == NULL) {
				map<string, Param>::iterator it = equMap.find(arg0->value);
				if (it != equMap.end()) {
					Param param = (*it).second;
					arg0->regAddress = param.regAddress;
					arg0->memAddress = param.memAddress;
				}
			}
			map<string, Memory*>::iterator it = memMap.find(arg0->value);
			if (it != memMap.end()) {
				MemoryAddress* memAddress = parseMemoryAddressWithLine(line, position, (*it).second);
				arg0->memAddress = memAddress;
			}

			if (arg0->regAddress == NULL && arg0->memAddress == NULL) {
				arg0->condition = fv1->conditionWithIdentifier(arg0->value);
				arg0->osc = fv1->oscillatorWithIdentifier(arg0->value);
				arg0->unsignedIntValue = arg0->osc;
			}

			map<string, Param>::iterator it3 = equMap.find(arg0->value);
			if (it3 != equMap.end()) {
				Param p = (*it3).second;
				arg0->doubleValue = negative ? p.doubleValue * -1.0 : p.doubleValue;
				arg0->unsignedIntValue = s23tohex(arg0->doubleValue);
			}

			map<string, unsigned int>::iterator it2 = labels.find(arg0->value);
			if (it2 != labels.end()) {
				unsigned int labelAddress = (*it2).second;
				arg0->doubleValue = labelAddress - currentInstruction;
			}
			
			// if the identifier is a CHO flag, then save all flags in this parameter
			if (isChoFlag(arg0->value)) {
				arg0->type = Lexer::TOKEN_TYPE::NUMBER;
				arg0->unsignedIntValue = getChoFlagWithString(arg0->value);
				if (line.size() > 0) {
					if (line[0]->type == Lexer::TOKEN_TYPE::VERTICAL_BAR) {
						line.erase(line.begin()); // erase vertical bar
						arg0->unsignedIntValue |= getChoFlagsValueWithLine(line);
					}
				}
			}
		}
		else if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER) {
			arg0->type = line[0]->type;
			arg0->value = line[0]->name;
			if (isHexIntegerValue(arg0->value)) {
				unsigned int hexValue = 0;
				STR2NUMBER_ERROR err = hexstr2uint(hexValue, arg0->value.c_str());
				if (err == SUCCESS) {
					arg0->unsignedIntValue = hexValue;
				}
			}
			else if (isBinaryS23Value(arg0->value)) {
				unsigned int hexValue = 0;
				string value = arg0->value.replace(0, 1, ""); // removed first character
				STR2NUMBER_ERROR err = binstr2uint(hexValue, value.c_str());
				if (err == SUCCESS) {
					arg0->doubleValue = hex2s23(hexValue);
					arg0->unsignedIntValue = hexValue;
				}

			}
			else if (isHexS23Value(arg0->value)) {
				unsigned int hexValue = 0;
				string value = arg0->value.replace(0, 1, "0x");
				STR2NUMBER_ERROR err = hexstr2uint(hexValue, value.c_str());
				if (err == SUCCESS) {
					arg0->doubleValue = hex2s23(hexValue);
					arg0->unsignedIntValue = hexValue;
				}
			}
			else {
				// attempt to read it as double value
				double coefficient = 0;
				STR2NUMBER_ERROR err = str2dble(coefficient, line[0]->name.c_str());
				if (err == SUCCESS) {
					arg0->doubleValue = negative ? coefficient * -1.0 : coefficient;
					arg0->unsignedIntValue = static_cast<u32>(arg0->doubleValue);
				}
			}
			line.erase(line.begin()); // erase number
		}
		else if (line[0]->type == Lexer::TOKEN_TYPE::COMMA) {
			arg0->doubleValue = 0.0;
			arg0->unsignedIntValue = 0;
		}
		else {
			throw exception("Invalid parameter type");
		}

		if (line.size() > 0) {
			Lexer::TOKEN_TYPE type = line[0]->type;
			if (type == Lexer::TOKEN_TYPE::COMMA) {
				line.erase(line.begin() + 0); // remove comma
			}
		}

		return arg0;
	}
	return NULL;
}

MemoryAddress* Parser::parseMemoryAddressWithLine(vector<Lexer::Token*>& line, MemoryPosition position, Memory* memory) {
	MemoryAddress* memAddress = new MemoryAddress();
	memAddress->mem = memory;
	memAddress->position = position;

	// memory address found, look for any modifier to the memory address
	if (line.size() > 1 && (line[0]->type == Lexer::TOKEN_TYPE::PLUS || line[0]->type == Lexer::TOKEN_TYPE::MINUS)) {
		// there is a displacement here
		bool negative = false;
		if (line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
			// is negative
			negative = true;
		}
		line.erase(line.begin()); // erase plus or minus token
		if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER) {
			unsigned int displacement = 0;
			STR2NUMBER_ERROR err = str2uint(displacement, line[0]->name.c_str());
			if (err == SUCCESS) {
				memAddress->displacement = negative ? displacement * -1 : displacement;
			}

			line.erase(line.begin()); // erase number
		}
		else if (line[0]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
			// displacement may be a constant declared before
			map<string, Param>::iterator it = equMap.find(line[0]->name);
			if (it != equMap.end() && (*it).second.type == Lexer::TOKEN_TYPE::NUMBER) {
				Param param = (*it).second;
				memAddress->displacement = static_cast<int>(param.doubleValue);
			}
			else {
				throw exception("constant expected");
			}

			line.erase(line.begin());
		}
		else {
			throw exception("a number was expected to construct a displacement.");
		}
	}

	return memAddress;
}

MemoryPosition Parser::DirectionSpecificationWithType(Lexer::TOKEN_TYPE& type) {
	switch (type) {
	case Lexer::TOKEN_TYPE::CIRCUMFLEX:
		return Middle;
	case Lexer::TOKEN_TYPE::NUMERAL:
		return End;
	default:
		return Start;
	}
}

Opcode Parser::InstructionOpcodeWithString(string& instruction) {
	if (_stricmp(instruction.c_str(), "rmpa") == 0) {
		return RMPA;
	}
	else if (_stricmp(instruction.c_str(), "rdax") == 0) {
		return RDAX;
	}
	else if (_stricmp(instruction.c_str(), "rda") == 0) {
		return RDA;
	}
	else if (_stricmp(instruction.c_str(), "wrap") == 0) {
		return WRAP;
	}
	else if (_stricmp(instruction.c_str(), "wrax") == 0) {
		return WRAX;
	}
	else if (_stricmp(instruction.c_str(), "wra") == 0) {
		return WRA;
	}
	else if (_stricmp(instruction.c_str(), "mulx") == 0) {
		return MULX;
	}
	else if (_stricmp(instruction.c_str(), "rdfx") == 0) {
		return RDFX;
	}
	else if (_stricmp(instruction.c_str(), "log") == 0) {
		return LOG;
	}
	else if (_stricmp(instruction.c_str(), "exp") == 0) {
		return EXP;
	}
	else if (_stricmp(instruction.c_str(), "sof") == 0) {
		return SOF;
	}
	else if (_stricmp(instruction.c_str(), "maxx") == 0) {
		return MAXX;
	}
	else if (_stricmp(instruction.c_str(), "ldax") == 0) {
		return LDAX;
	}
	else if (_stricmp(instruction.c_str(), "skp") == 0) {
		return SKP;
	}
	else if (_stricmp(instruction.c_str(), "absa") == 0) {
		return ABSA;
	}
	else if (_stricmp(instruction.c_str(), "wrlx") == 0) {
		return WRLX;
	}
	else if (_stricmp(instruction.c_str(), "wrhx") == 0) {
		return WRHX;
	}
	else if (_stricmp(instruction.c_str(), "wlds") == 0) {
		return WLDS;
	}
	else if (_stricmp(instruction.c_str(), "wldr") == 0) {
		return WLDR;
	}
	else if (_stricmp(instruction.c_str(), "cho") == 0) {
		return CHO;
	}
	else if (_stricmp(instruction.c_str(), "or") == 0) {
		return OR;
	}
	else if (_stricmp(instruction.c_str(), "and") == 0) {
		return AND;
	}
	else if (_stricmp(instruction.c_str(), "xor") == 0) {
		return XOR;
	}
	else {
		return UNKNOWN;
	}
}
// separate statements that should be executed on first pass, and second
// this handles label declarations followed by code
ExecutionVectorResult Parser::generateExecutionVector(vector<vector<Lexer::Token*>> lines, vector<string> linesStr) {

	ExecutionVectorResult exec = ExecutionVectorResult();

	for (unsigned int i = 0; i < lines.size(); i++) {
		vector<Lexer::Token*> v = lines[i];
		string lineStr = linesStr[i];

		SplitStatementInfo splitInfo = shouldSplitStatements(v);
		if (splitInfo.shouldSplit) {
			string label = splitInfo.labelDeclared;
			unsigned int absoluteCodeLine = exec.secondPass.size() - 1; // zero based line
			labels[label] = absoluteCodeLine;

			if (splitInfo.secondPassStatement.size() > 0) {
				exec.secondPass.push_back(splitInfo.secondPassStatement);
				exec.secondPassRawLines.push_back(lineStr);
			}
		}
		else {
			if (isFirstPassStatement(v)) {
				exec.firstPass.push_back(v);
				exec.firstPassRawLines.push_back(lineStr);
			}
			else {
				exec.secondPass.push_back(v);
				exec.secondPassRawLines.push_back(lineStr);
			}
		}
	}
	exec.success = true;
	return exec;
}

// splits instructions to be parsed in different stages
// label declarations should be executed in first pass,
// the same line can be followed by code.
SplitStatementInfo Parser::shouldSplitStatements(vector<Lexer::Token*> line) {
	SplitStatementInfo splitInfo = SplitStatementInfo();
	splitInfo.shouldSplit = false;

	if (line.size() > 1) {
		Lexer::TOKEN_TYPE type = line[0]->type;
		if (type == Lexer::TOKEN_TYPE::IDENTIFIER) {
			// if followed by (:) colon, is a label, should be added to first group
			if (line[1]->type == Lexer::TOKEN_TYPE::COLON) {
				splitInfo.shouldSplit = true;
				splitInfo.labelDeclared = line[0]->name;

				if (line.size() > 2) {
					for (unsigned int i = 2; i < line.size(); i++) {
						splitInfo.secondPassStatement.push_back(line[i]);
					}
				}
			}
		}
	}
	return splitInfo;
}
BOOL Parser::isFirstPassStatement(vector<Lexer::Token*> v) {
	Lexer::TOKEN_TYPE type = v[0]->type;
	if (type == Lexer::TOKEN_TYPE::IDENTIFIER) {
		string id = v[0]->name;
		if (_stricmp(id.c_str(), "mem") == 0) {
			return true;
		}
		else if (_stricmp(id.c_str(), "equ") == 0) {
			return true;
		}
	}
	return false;
}

Opcode Parser::opcodeWithSecondaryOpcode(Opcode opcode, string subOpcode) {
	if (opcode == CHO) {
		if (_stricmp(subOpcode.c_str(), "rda") == 0) {
			return CHO_RDA;
		}
		else if (_stricmp(subOpcode.c_str(), "sof") == 0) {
			return CHO_SOF;
		}
		else if (_stricmp(subOpcode.c_str(), "rdal") == 0) {
			return CHO_RDAL;
		}
	}

	assert(false); // invalid arguments
	
}

BOOL Parser::isChoFlag(string id) {
	return getChoFlagWithString(id) != CHOFlags::UNKNOWN_CHO_FLAG;
}

unsigned int Parser::getChoFlagWithString(string id) {
	const char* str = id.c_str();
	if (_stricmp(str, "sin") == 0) {
		return CHOFlags::SIN;
	}
	else if (_stricmp(str, "cos") == 0) {
		return CHOFlags::COS;
	} 
	else if (_stricmp(str, "compa") == 0) {
		return CHOFlags::COMPA;
	}
	else if (_stricmp(str, "compc") == 0) {
		return CHOFlags::COMPC;
	}
	else if (_stricmp(str, "reg") == 0) {
		return CHOFlags::REG;
	}
	else if (_stricmp(str, "na") == 0) {
		return CHOFlags::NA;
	}
	else if (_stricmp(str, "rptr2") == 0) {
		return CHOFlags::RPTR2;
	}

	return CHOFlags::UNKNOWN_CHO_FLAG;
}

unsigned int Parser::getChoFlagsValueWithLine(vector<Lexer::Token*>& line) {
	unsigned int choFlags = CHOFlags::UNKNOWN_CHO_FLAG;
	if (line.size() > 0) {
		if (line[0]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
			string value = line[0]->name;
			if (isChoFlag(value)) {
				choFlags = getChoFlagWithString(value);
				line.erase(line.begin() + 0); // remove flag from line

				// look ahead for a vertical bar
				if (line.size() > 0) {
					if (line[0]->type == Lexer::TOKEN_TYPE::VERTICAL_BAR) {
						line.erase(line.begin() + 0); // remove vertical bar
						choFlags |= getChoFlagsValueWithLine(line);
					}
				}
			}
			else {
				assert(false); // cho flag expected;
			}
		}
		else {
			assert(false); // identifier expected
		}
	}
	else {
		// no error, finalized parse of flags.
	}

	return choFlags;
}

// value should start with 0 and x
BOOL Parser::isHexIntegerValue(string value) {
	
	if (value.size() > 2) {
		if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Parser::isHexS23Value(string value) {
	if (value.size() > 1) {
		if (value[0] == '$') {
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Parser::isBinaryS23Value(string value) {
	if (value.size() > 1) {
		if (value[0] == '%') {
			return TRUE;
		}
	}
	return FALSE;
}