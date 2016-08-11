#include "stdafx.h"
#include "Parser.h"
#include "..\Utilities\ParseUtil.h"
#include "..\Manager\MemoryManager.h"
#include "..\Utilities\SCLog.h"

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
		if (v.size() == 3) {
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
						}
						else if (v[2]->type == Lexer::TOKEN_TYPE::NUMBER) {
							double coeff = 0;
							STR2NUMBER_ERROR err = str2dble(coeff, v[2]->name.c_str());
							if (err == SUCCESS) {
								equ.doubleValue = coeff;
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
			if (opcode != UNKNOWN) {
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

vector<Param*> Parser::GetParameters(vector<Lexer::Token*> line, unsigned int currentInstruction) {
	vector<Param*> parameters;
	Param* param = GetParameter(line, currentInstruction);
	if (param != NULL) {
		parameters.push_back(param);
		if (line.size() > 0) {
			Lexer::TOKEN_TYPE type = line[0]->type;
			if (type == Lexer::TOKEN_TYPE::COMMA) {
				line.erase(line.begin() + 0); // remove comma
				vector<Param*> params = GetParameters(line, currentInstruction);
				if (params.size() > 0) {
					parameters.insert(parameters.end(), params.begin(), params.end());
				}
			}
		}
	}
	
	return parameters;
}

Param* Parser::GetParameter(vector<Lexer::Token*>& line, unsigned int currentInstruction) {
	if (line.size() > 0) {
		FV1::MemoryPosition position = FV1::Start;
		bool negative = false;
		Param* arg0 = new Param();
		
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
			arg0->dir = position;

			arg0->regAddress = fv1->getAddressOfIdentifier(arg0->value);
			if (arg0->regAddress == NULL) {
				map<string, Param>::iterator it = equMap.find(arg0->value);
				if (it != equMap.end()) {
					Param param = (*it).second;
					arg0->regAddress = param.regAddress;
				}
			}
			map<string, Memory*>::iterator it = memMap.find(arg0->value);
			if (it != memMap.end()) {
				MemoryAddress* memAddress = new MemoryAddress();
				memAddress->mem = (*it).second;
				// memory address found, look for any modifier to the memory address
				if (line.size() > 3 && (line[0]->type == Lexer::TOKEN_TYPE::PLUS || line[0]->type == Lexer::TOKEN_TYPE::MINUS)) {
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
					else {
						throw exception("a number was expected to construct a displacement.");
					}
				}

				arg0->memAddress = memAddress;
			}

			if (arg0->regAddress == NULL && arg0->memAddress == NULL) {
				arg0->condition = fv1->conditionWithIdentifier(arg0->value);
				arg0->osc = fv1->oscillatorWithIdentifier(arg0->value);
			}

			map<string, Param>::iterator it3 = equMap.find(arg0->value);
			if (it3 != equMap.end()) {
				Param p = (*it3).second;
				arg0->doubleValue = negative ? p.doubleValue * -1.0 : p.doubleValue;
			}

			map<string, unsigned int>::iterator it2 = labels.find(arg0->value);
			if (it2 != labels.end()) {
				unsigned int labelAddress = (*it2).second;
				arg0->doubleValue = labelAddress - currentInstruction;
			}

		}
		else if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER) {
			arg0->type = line[0]->type;
			arg0->value = line[0]->name;
			double coefficient = 0;
			STR2NUMBER_ERROR err = str2dble(coefficient, line[0]->name.c_str());
			if (err == SUCCESS) {
				arg0->doubleValue = negative ? coefficient * -1.0 : coefficient;
			}
			line.erase(line.begin()); // erase number
		}
		else {
			throw exception("Invalid parameter type");
		}
		return arg0;
	}
	return NULL;
}

FV1::MemoryPosition Parser::DirectionSpecificationWithType(Lexer::TOKEN_TYPE& type) {
	switch (type) {
	case Lexer::TOKEN_TYPE::CIRCUMFLEX:
		return FV1::Middle;
	case Lexer::TOKEN_TYPE::NUMERAL:
		return FV1::End;
	default:
		return FV1::Start;
	}
}

Opcode Parser::InstructionOpcodeWithString(string& instruction) {
	if (instruction.compare("rdax") == 0) {
		return RDAX;
	}
	else if (instruction.compare("rda") == 0) {
		return RDA;
	}
	else if (instruction.compare("wrap") == 0) {
		return WRAP;
	}
	else if (instruction.compare("wrax") == 0) {
		return WRAX;
	}
	else if (instruction.compare("wra") == 0) {
		return WRA;
	}
	else if (instruction.compare("mulx") == 0) {
		return MULX;
	}
	else if (instruction.compare("rdfx") == 0) {
		return RDFX;
	}
	else if (instruction.compare("log") == 0) {
		return LOG;
	}
	else if (instruction.compare("exp") == 0) {
		return EXP;
	}
	else if (instruction.compare("sof") == 0) {
		return SOF;
	}
	else if (instruction.compare("maxx") == 0) {
		return MAXX;
	}
	else if (instruction.compare("ldax") == 0) {
		return LDAX;
	}
	else if (instruction.compare("skp") == 0) {
		return SKP;
	}
	else if (instruction.compare("absa") == 0) {
		return ABSA;
	}
	else if (instruction.compare("wrlx") == 0) {
		return WRLX;
	}
	else if (instruction.compare("wrhx") == 0) {
		return WRHX;
	}
	else if (instruction.compare("wlds") == 0) {
		return WLDS;
	}
	else if (instruction.compare("cho") == 0) {
		return CHO;
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
		if (id.compare("mem") == 0) {
			return true;
		}
		else if (id.compare("equ") == 0) {
			return true;
		}
	}
	return false;
}