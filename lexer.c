#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

Token tokens[MAX_TOKENS];
int nTokens;

int line = 1;		// the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token* addTk(int code) {
	if (nTokens == MAX_TOKENS)err("too many tokens");
	Token* tk = &tokens[nTokens];
	tk->code = code;
	tk->line = line;
	nTokens++;
	return tk;
}

// copy in the dst buffer the string between [begin,end)
char* copyn(char* dst, const char* begin, const char* end) {
	char* p = dst;
	if (end - begin > MAX_STR)err("string too long");
	while (begin != end)*p++ = *begin++;
	*p = '\0';
	return dst;
}

// clear buf
void clearBuf(char* buf) {
	memset(buf, 0, MAX_STR + 1);
}

void tokenize(const char* pch) {
	const char* start;
	Token* tk;
	char buf[MAX_STR + 1];
	clearBuf(buf);
	for (;;) {
		switch (*pch) {
		case ' ':case '\t': pch++; break;
		case '\r':		// handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
			if (pch[1] == '\n')pch++;
			// fallthrough to \n
		case '\n':
			line++; pch++; break;
		case '\0': addTk(FINISH); return;
		case ':': addTk(COLON); pch++; break;
		case ';': addTk(SEMICOLON); pch++; break;
		case '(': addTk(LPAR); pch++; break;
		case ')': addTk(RPAR); pch++; break;
		case ',': addTk(COMMA); pch++; break;
		case '+': addTk(ADD); pch++; break;
		case '-': addTk(SUB); pch++; break;
		case '*': addTk(MUL); pch++; break;
		case '/': addTk(DIV); pch++; break;
		case '<': addTk(LESS); pch++; break;
		case '>':
			if (pch[1] == '=') {
				addTk(GREATEREQ);
				pch += 2;
			}
			else {
				addTk(GREATER);
				pch++;
			}
			break;
		case '=':
			if (pch[1] == '=') {
				addTk(EQUAL);
				pch += 2;
			}
			else {
				addTk(ASSIGN);
				pch++;
			}
			break;
		case '|':
			if (pch[1] == '|') {
				addTk(OR);
				pch += 2;
			}
			else {
				err("invalid operator: %c(%d) %c(%d)", *pch, *pch, *(pch + 1), *(pch + 1));
			}
			break;
		case '!':
			if (pch[1] == '=') {
				addTk(NOTEQ);
				pch += 2;
			}
			else {
				addTk(NOT);
				pch++;
			}
			break;
		case '"':	// string constant
			for (start = ++pch; *pch != '"'; pch++) {}
			tk = addTk(STR);
			copyn(tk->text, start, pch++);
			break;
		case '#': // discard comments
			for (*pch++; *pch != '\n'; pch++) {}
			break;
		default:	// multi-char: types, identifiers, operators, keywords
			if (isalpha(*pch) || *pch == '_') {
				for (start = pch++; isalnum(*pch) || *pch == '_'; pch++) {}
				char* text = copyn(buf, start, pch);
				if (strcmp(text, "int") == 0) addTk(TYPE_INT);
				else if (strcmp(text, "real") == 0) addTk(TYPE_REAL);
				else if (strcmp(text, "str") == 0) addTk(TYPE_STR);

				else if (strcmp(text, "var") == 0) addTk(VAR);
				else if (strcmp(text, "function") == 0) addTk(FUNCTION);
				else if (strcmp(text, "if") == 0) addTk(IF);
				else if (strcmp(text, "else") == 0) addTk(ELSE);
				else if (strcmp(text, "while") == 0) addTk(WHILE);
				else if (strcmp(text, "end") == 0) addTk(END);
				else if (strcmp(text, "return") == 0) addTk(RETURN);
				else {	// identifier
					tk = addTk(ID);
					strcpy(tk->text, text);
				}
			}
			else if (isdigit(*pch)) {	// numeric constants 
				for (start = pch++; isdigit(*pch); pch++) {}
				if (*pch == '.') {	// REAL
					for (pch++; isdigit(*pch); pch++) {}
					tk = addTk(REAL);
					clearBuf(buf);
					char* real_str = copyn(buf, start, pch);
					tk->r = strtod(real_str, NULL);
				}
				else {
					tk = addTk(INT);
					clearBuf(buf);
					char* int_str = copyn(buf, start, pch);
					tk->i = atoi(int_str);
				}
			}
			else err("invalid char: %c (%d)", *pch, *pch);
		}
	}
}

void showTokens() {
	for (int i = 0; i < nTokens; i++) {
		Token* tk = &tokens[i];
		printf("%d: %s", tk->line, tkCodeName(tk->code));

		switch (tk->code) {
			case ID: case STR: printf(": %s\n", tk->text); break;
			case INT: printf(": %d\n", tk->i); break;
			case REAL: printf(": %f\n", tk->r); break;
			default: printf("\n");
		}
	}
}
