#pragma once



enum {
	// identifiers
	ID = 0,

	// keywords
	VAR = 1,
	FUNCTION = 2,
	IF = 3,
	ELSE = 4,
	WHILE = 5,
	END = 6,
	RETURN = 7,
	TYPE_INT = 8,
	TYPE_REAL = 9,
	TYPE_STR = 10,

	// delimiters
	COMMA = 11,
	COLON = 12,
	SEMICOLON = 13,
	LPAR = 14,
	RPAR = 15,
	FINISH = 16,

	// operators
	ADD = 17,
	SUB = 18,
	MUL = 19,
	DIV = 20,
	AND = 21,
	OR = 22,
	NOT = 23,
	ASSIGN = 24,
	EQUAL = 25,
	NOTEQ = 26,
	LESS = 27,
	GREATER = 28,
	GREATEREQ = 29,

	// constants
	INT = 30,
	REAL = 31,
	STR = 32
};

static inline char* tkCodeName (int code)
{
	static const char* strings[] = { 
		"ID",

		"VAR", 
		"FUNCTION", 
		"IF", 
		"ELSE", 
		"WHILE", 
		"END", 
		"RETURN", 
		"TYPE_INT", 
		"TYPE_REAL", 
		"TYPE_STR",

		"COMMA",
		"COLON",
		"SEMICOLON",
		"LPAR",
		"RPAR",
		"FINISH",

		"ADD",
		"SUB",
		"MUL",
		"DIV",
		"AND",
		"OR",
		"NOT",
		"ASSIGN",
		"EQUAL",
		"NOTEQ",
		"LESS",
		"GREATER",
		"GREATEREQ",

		"INT",
		"REAL",
		"STR"
	};

	return strings[code];
}

#define MAX_STR		127

typedef struct {
	int code;		// ID, TYPE_INT, ...
	int line;		// the line from the input file
	union {
		char text[MAX_STR + 1];		// the chars for ID, STR
		int i;		// the value for INT
		double r;		// the value for REAL
	};
} Token;
 
#define MAX_TOKENS		4096
extern Token tokens[];
extern int nTokens;

void tokenize(const char* pch);
void showTokens();