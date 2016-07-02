#include "stdafx.h"
#include "Lexer.h"

Lexer::Lexer(void* lpBuffer, unsigned int size) {

	optable['+'] = PLUS;
	optable['-'] = MINUS;
	optable[','] = COMMA;
	optable['#'] = NUMERAL;
	optable[':'] = COLON;
	optable['^'] = CIRCUMFLEX;
	optable['\n'] = NEWLINE;
	optable['\r'] = NEWLINE;
	optable['|'] = VERTICAL_BAR;

	this->buflen = size;
	this->lpBuffer = lpBuffer;
	this->pos = 0;
}

bool Lexer::hasNextLine() {
	if (pos < buflen) {
		return true;
	}
	return false;
}
bool Lexer::hasNextToken() {
	if (pos < buflen) {
		return true;
	}
	return false;
}

string Lexer::getNextLine() {
	string line;
	int tempPos = pos;
	while (tempPos < buflen) {
		char c = ((char*)lpBuffer)[tempPos];
		if (isNewLine(c)) {
			break;
		}
		line.push_back(c);
		tempPos++;
	}
	return line;
}

vector<Lexer::Token*> Lexer::getTokenizedNextLine() {
	vector<Token*> v;
	while (hasNextToken()) {
		Token* token = getNextToken();
		if (token != NULL) {
			if (token->type == NEWLINE || token->type == COMMENT) {
				delete token;				// not used token
				return v;
			}
			v.push_back(token);
		}
	}
	return v;
}

Lexer::Token* Lexer::getNextToken() {
	Token* token = NULL;

	this->skipNonTokens();

	if (this->pos >= this->buflen) {
		return NULL;
	}

	char c = ((char*)lpBuffer)[pos];

	// is comment?
	if (c == ';') {
		return this->processComments();
	}
	else {
		// Look it up in the table of operators
		map<char, TOKEN_TYPE>::iterator op = optable.find(c);
		if (op != optable.end()) {
			TOKEN_TYPE type = (*op).second;
			token = new Token();
			token->name.push_back(c);
			token->type = type;
			token->pos = pos++;
			return token;
		}
		else {
			// Not an operator - so it's the beginning of another token.
			if (isalpha(c)) {
				return this->process_identifier();
			}
			else if (isdigit(c)) {
				return this->process_number();
			}
			else {
				throw std::exception("Token error at ");
			}

		}
	}
	return token;
}

void Lexer::skipNonTokens() {
	while (pos < buflen) {
		char c = ((char*)lpBuffer)[pos];
		if (c == ' ' || c == '\t') {
			pos++;
		}
		else {
			break;
		}
	}
}

Lexer::Token* Lexer::processComments() {
	Token* token = new Token();
	token->type = Lexer::TOKEN_TYPE::COMMENT;
	pos++; // consume semicolon

	while (pos < buflen) {
		char c = ((char*)lpBuffer)[pos];
		if (isNewLine(c)) {
			break;
		}
		token->name.push_back(c);
		pos++;
	}

	return token;
}

bool Lexer::isNewLine(char c) {
	return c == '\n' || c == '\r';
}

Lexer::Token* Lexer::process_identifier() {

	Token* tok = new Token();
	tok->type = IDENTIFIER;

	char c = ((char*)lpBuffer)[pos];
	tok->name.push_back(c);
	pos++;

	while (pos < buflen) {
		char c = ((char*)lpBuffer)[pos];
		if (!isalnum(c)) {
			break;
		}
		tok->name.push_back(c);
		pos++;
	}

	tok->pos = pos;
	return tok;
}

Lexer::Token* Lexer::process_number() {
	Token* tok = new Token();
	tok->type = NUMBER;

	char c = ((char*)lpBuffer)[pos];
	tok->name.push_back(c);

	pos++;
	while (pos < buflen) {
		char c = ((char*)lpBuffer)[pos];
		if (c != '.' && !isdigit(c)) {
			break;
		}
		tok->name.push_back(c);
		pos++;
	}
	tok->pos = pos;
	return tok;
}