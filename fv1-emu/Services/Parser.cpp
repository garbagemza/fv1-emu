#include "stdafx.h"
#include "Parser.h"
#include "..\Utilities\ParseUtil.h"
#include "..\Manager\MemoryManager.h"
#include "..\Utilities\SCLog.h"

ExecutionVectorResult beginLexicalAnalysis(LPVOID lpBuffer, DWORD size) {
	Lexer lex = Lexer(lpBuffer, size);
	vector<vector<Lexer::Token*>> lines;
	while (lex.hasNextLine()) {
		string lineStr;
		try {
			lineStr = lex.getNextLine();
			if (lineStr.size() > 0) {
				SCLogInfo(L"Parsing line: %s", lineStr.c_str())
			}
			vector<Lexer::Token*> line = lex.getTokenizedNextLine();
			if (line.size() > 0) {
				lines.push_back(line);
			}
		}
		catch (std::exception& e) {
			SCLogError(L"Unable to parse: %s, exception: %s", lineStr.c_str(), e.what());
			ExecutionVectorResult err = ExecutionVectorResult();
			err.success = false;
			return err;
		}
	}

	if (lines.size() > 0) {
		// ok, the lexical analysis has been successful
		return generateExecutionVector(lines);
	}

	ExecutionVectorResult err = ExecutionVectorResult();
	err.success = false;
	return err;
}

// first pass is to identify memory declarations and equates
// initializes memory and equates dictionaries
PassOneResult PassOneParse(FV1* fv1, vector<vector<Lexer::Token*>> lines) {
	PassOneResult result = PassOneResult();
	result.success = false;


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
								result.memoryMap[v[1]->name] = memory;

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
						result.equatesMap[v[1]->name] = equ;
					}
				}
			}
		}

	}
	result.success = true;
	return result;
}


PassTwoResult PassTwoParse(FV1* fv1, map<string, Param> equMap, map<string, Memory*> memMap, map<string, unsigned int> labels, vector<vector<Lexer::Token*>> lines) {
	PassTwoResult result = PassTwoResult();
	result.success = false;
	unsigned int index = 0;
	for (vector<vector<Lexer::Token*>>::iterator it = lines.begin(); it != lines.end(); it++) {
		vector<Lexer::Token*> v = (*it);
		Instruction* inst = new Instruction();
		BOOL instructionLoaded = LoadInstructionWithInstructionLine(fv1, equMap, memMap, labels, v, index, inst);
		if (instructionLoaded) {
			result.instructions[index] = inst;
		}
		else {
			return result;
		}
		index++;
	}

	result.instructionCount = index;
	result.success = true;
	return result;
}

BOOL LoadInstructionWithInstructionLine(FV1* fv1, map<string, Param> equMap, map<string, Memory*> memMap, map<string, unsigned int> labels, vector<Lexer::Token*> line, unsigned int currentInstruction, Instruction* instruction) {
	if (line.size() > 0) {
		Lexer::TOKEN_TYPE type = line[0]->type;
		if (type == Lexer::TOKEN_TYPE::IDENTIFIER) {
			string inst = line[0]->name;
			InstructionType type = InstructionTypeWithString(inst);
			if (type != UNKNOWN) {
				line.erase(line.begin() + 0); // remove first element
				instruction->type = type;

				if (line.size() > 2) {

					FV1::MemoryPosition position = FV1::Start;
					Param* arg0 = new Param();

					if (line[0]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
						arg0->type = line[0]->type;
						arg0->value = line[0]->name;

						line.erase(line.begin());

						// next token is optional, it can be numeral (#) or circumflex (^)
						// if followed by a comma, the direction specification is the default.
						if (line[0]->type == Lexer::TOKEN_TYPE::NUMERAL || line[0]->type == Lexer::TOKEN_TYPE::CIRCUMFLEX) {
							position = DirectionSpecificationWithType(line[0]->type);
							line.erase(line.begin());
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
						}
					}
					else if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER || line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
						// log and exp should have as first argument a number
						bool negative = false;
						if (line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
							line.erase(line.begin());
							negative = true;
						}

						if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER) {
							arg0->type = line[0]->type;
							arg0->value = line[0]->name;
							double coefficient = 0;
							STR2NUMBER_ERROR err = str2dble(coefficient, line[0]->name.c_str());
							if (err == SUCCESS) {
								arg0->doubleValue = negative ? coefficient * -1.0 : coefficient;
							}
							line.erase(line.begin()); // erase number
						}
					}


					if (instruction->type == RDAX || instruction->type == WRAX || instruction->type == MAXX || instruction->type == WRLX) {
						if (arg0->regAddress == NULL) {
							throw exception("register address is required for first argument.");
						}
					}

					if (instruction->type == RDA || instruction->type == WRA || instruction->type == WRAP) {
						if (arg0->memAddress == NULL) {
							throw exception("memory is required for first argument.");
						}
					}

					if (instruction->type == SKP) {
						if (arg0->condition == 0) {
							throw exception("condition is required for first argument.");
						}
					}

					instruction->arg0 = arg0;

					// next token is a comma (followed by a second argument)
					if (line.size() > 1) {
						if (line[0]->type == Lexer::TOKEN_TYPE::COMMA) {
							line.erase(line.begin()); // consume the comma
							bool negative = false;
							// parse second argument
							// next is optional, it can be a negative flag
							if (line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
								negative = true;
								line.erase(line.begin()); // consume the negative
							}

							if (line.size() > 0) {
								Param* arg1 = new Param();
								double coefficient = 0;
								if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER) {
									STR2NUMBER_ERROR err = str2dble(coefficient, line[0]->name.c_str());
									if (err == SUCCESS) {
										arg1->doubleValue = negative ? coefficient * -1.0 : coefficient;

										arg1->skipLines = unsigned int(arg0->doubleValue);
									}
								}
								else if (line[0]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
									map<string, Param>::iterator it = equMap.find(line[0]->name);
									if (it != equMap.end()) {
										Param p = (*it).second;
										arg1->doubleValue = negative ? p.doubleValue * -1.0 : p.doubleValue;
									}

									map<string, unsigned int>::iterator it2 = labels.find(line[0]->name);
									if (it2 != labels.end()) {
										unsigned int labelAddress = (*it2).second;
										arg1->skipLines = labelAddress - currentInstruction;
									}
								}
								arg1->type = line[0]->type;
								arg1->value = line[0]->name;

								instruction->arg1 = arg1;

								line.erase(line.begin());
								// next is optional, is a comma, followed by a third argument
								if (line.size() > 1) {
									if (line[0]->type == Lexer::TOKEN_TYPE::COMMA) {
										Param* arg2 = new Param();

										line.erase(line.begin());
										bool negative = false;
										if (line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
											line.erase(line.begin());
											negative = true;
										}

										if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER) {
											arg2->type = line[0]->type;
											arg2->value = line[0]->name;

											double coefficient = 0;
											STR2NUMBER_ERROR err = str2dble(coefficient, line[0]->name.c_str());
											if (err == SUCCESS) {
												arg2->doubleValue = negative ? coefficient * -1.0 : coefficient;
											}
											line.erase(line.begin());
										}
										instruction->arg2 = arg2;
									}
								}

								return true;
							}
						}
					}
				}
				else if (line.size() > 0) {
					// this instruction has one argument, like mulx, ldax
					if (line[0]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {

						Param* arg0 = new Param();
						arg0->type = line[0]->type;
						arg0->value = line[0]->name;
						arg0->dir = FV1::Start;
						arg0->regAddress = fv1->getAddressOfIdentifier(arg0->value);
						if (arg0->regAddress == NULL) {
							map<string, Param>::iterator it = equMap.find(arg0->value);
							if (it != equMap.end()) {
								Param param = (*it).second;
								arg0->regAddress = param.regAddress;
							}
						}

						if (instruction->type == MULX || instruction->type == LDAX) {
							if (arg0->regAddress == NULL) {
								throw exception("register address is required for first argument.");
							}
						}

						instruction->arg0 = arg0;
						instruction->arg1 = NULL;
						return true;
					}
				}
				else {
					// 0 arguments, it must be absa
					instruction->arg0 = NULL;
					instruction->arg1 = NULL;
					return true;
				}
			}
			else {
				throw exception("unknown instruction.");
			}
		}
	}
	else {
		throw exception("instruction too small.");
	}
	return false;
}

FV1::MemoryPosition DirectionSpecificationWithType(Lexer::TOKEN_TYPE& type) {
	switch (type) {
	case Lexer::TOKEN_TYPE::CIRCUMFLEX:
		return FV1::Middle;
	case Lexer::TOKEN_TYPE::NUMERAL:
		return FV1::End;
	default:
		return FV1::Start;
	}
}

InstructionType InstructionTypeWithString(string& instruction) {
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
ExecutionVectorResult generateExecutionVector(vector<vector<Lexer::Token*>> lines) {

	ExecutionVectorResult exec = ExecutionVectorResult();

	for (vector<vector<Lexer::Token*>>::iterator it = lines.begin(); it != lines.end(); it++) {
		vector<Lexer::Token*> v = (*it);
		SplitStatementInfo splitInfo = shouldSplitStatements(v);
		if (splitInfo.shouldSplit) {
			string label = splitInfo.labelDeclared;
			unsigned int absoluteCodeLine = exec.secondPass.size() - 1; // zero based line
			exec.labels[label] = absoluteCodeLine;

			if (splitInfo.secondPassStatement.size() > 0) {
				exec.secondPass.push_back(splitInfo.secondPassStatement);
			}
		}
		else {
			if (isFirstPassStatement(v)) {
				exec.firstPass.push_back(v);
			}
			else {
				exec.secondPass.push_back(v);
			}
		}
	}
	exec.success = true;
	return exec;
}

// splits instructions to be parsed in different stages
// label declarations should be executed in first pass,
// the same line can be followed by code.
SplitStatementInfo shouldSplitStatements(vector<Lexer::Token*> line) {
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
BOOL isFirstPassStatement(vector<Lexer::Token*> v) {
	if (v.size() == 3) {
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
	}

	return false;
}