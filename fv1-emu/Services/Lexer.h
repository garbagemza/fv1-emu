#pragma once
#include <map>
#include <string>
#include <vector>

using namespace std;


class Lexer {


public:

	enum TOKEN_TYPE {
		PLUS,
		MINUS,
		COMMA,
		NUMERAL,
		COLON,
		CIRCUMFLEX,
		VERTICAL_BAR,
		NEWLINE,
		COMMENT,
		IDENTIFIER,
		NUMBER,
		LESS_THAN,
		GREATER_THAN
	};

	struct Token
	{
		Lexer::TOKEN_TYPE type;
		string name;
		unsigned int pos;
	};

	Lexer(void* lpBuffer, unsigned int size);

	bool hasNextLine();
	string getNextLine();
	vector<Token*> getTokenizedNextLine();
	

private:
	bool hasNextToken();
	bool isAlpha(char c);
	bool isAlNum(char c);
	bool isDigitOrDot(char c);
	bool isHexDigit(char c);

	Token* getNextToken();
	void skipNonTokens();
	Token* processComments();
	Token* process_identifier();
	Token* process_number();

	bool isNewLine(char c);

	map<char, TOKEN_TYPE> optable;
	void* lpBuffer;
	unsigned int buflen;
	unsigned int pos;
	unsigned int line;
};