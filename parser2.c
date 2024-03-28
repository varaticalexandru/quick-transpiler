#define _CRT_SECURE_NO_WARNINGS

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"
#include "parser2.h"
#include "ad.h"
#include "at.h"
#include "gen.h"

int iTk;	// the iterator in tokens
Token* consumed;	// the last consumed token

// same as err, but also prints the line of the current token
_Noreturn; void tkerr(const char* fmt, ...) {
	fprintf(stderr, "error in line %d: ", tokens[iTk].line);
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}


bool consume(int code) {
	printf("consume (%s)", tkCodeName(code));

	if (tokens[iTk].code == code) {	// if the current token is of type code
		consumed = &tokens[iTk++];	// set consumed to the current token and increment iTk
		printf(" => consumed\n");
		return true;
	}

	printf(" => found %s\n", tkCodeName(tokens[iTk].code));
	return false;
}

void parse() {
	iTk = 0;
	program();
}

// program ::= ( defVar | defFunc | block )* FINISH
bool program() {
	printf("\n------- FIRS: program -------\n");

	addDomain(); // create global domain

	addPredefinedFns();	// add predefined functions

	crtCode = &tMain; // set the current code to tMain
	crtVar = &tBegin; // set the current variable to tBegin

	Text_write(&tBegin, "#include \"quick.h\"\n\n");	// write the header of the C file
	Text_write(&tMain, "\nint main(){\n");	// write the header of the main function

	while (true) {
		if (defVar()) {}
		else if (defFunc()) {}
		else if (block()) {}
		else break;
	}

	if (consume(FINISH)) {
		delDomain(); // delete global domain
		printf("\n------- END: program -------\n");
		printf("\n\n------- Syntax = OK -------\n");

		Text_write(&tMain, "\nreturn 0;\n}\n");	// write the footer of the main function
		FILE* file = fopen("transpiled_1.c", "w");	// open the file 1.c
		if (!file) {
			puts("cannot open transpiled_1.c");
			exit(EXIT_FAILURE);
		}

		fwrite(tBegin.buf, sizeof(char), tBegin.n, file);	// write the header of the C file
		fwrite(tFunctions.buf, sizeof(char), tFunctions.n, file);	// write the main function
		fwrite(tMain.buf, sizeof(char), tMain.n, file);	// write the main function
		fclose(file);	// close the file 1.c

		return false;
	}
	else tkerr("syntax error");

	return false;
}


// defVar ::= VAR ID COLON baseType SEMICOLON
bool defVar() {

	printf("\n------- FIRS: defVar -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (consume(VAR)) {
		if (consume(ID)) {
			// check if the variable is already defined in the current domain
			const char* name = consumed->text;
			Symbol* s = searchInCurrentDomain(name);
			if (s) tkerr("domain error: variable redefinition: %s", name);

			// add the variable to the current domain
			s = addSymbol(name, KIND_VAR);
			s->local = crtFn != NULL;	// if crtFn is not NULL, then the variable is local

			if (consume(COLON)) {
				if (baseType()) {
					s->type = ret.type;	// set the type of the variable
					if (consume(SEMICOLON)) {

						Text_write(crtVar, "%s %s;\n", cType(s->type), s->name);	// write the variable in the current variable buffer

						printf("\n------- END: defVar -------\n");
						return true;
					}
					else tkerr("syntax error: missing ';' after [base type]");
				}
				else tkerr("syntax error: missing [base type] after ':'");
			}
			else tkerr("syntax error: missing ':' after [identifier]");
		}
		else tkerr("syntax error: missing [identifier] after 'var'");
	}
	else {
		printf("\n------- END: defVar -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}


// baseType ::= TYPE_INT | TYPE_REAL | TYPE_STR
bool baseType() {

	printf("\n------- FIRS: baseType -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (consume(TYPE_INT)) {
		ret.type = TYPE_INT;	// set the type of the variable
		printf("\n------- END: baseType -------\n");
		return true;
	}
	else if (consume(TYPE_REAL)) {
		ret.type = TYPE_REAL;	// set the type of the variable
		printf("\n------- END: baseType -------\n");
		return true;
	}
	else if (consume(TYPE_STR)) {
		ret.type = TYPE_STR;	// set the type of the variable
		printf("\n------- END: baseType -------\n");
		return true;
	}
	else {
		printf("\n------- END: baseType -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}


// defFunc ::= FUNCTION ID LPAR funcParams RPAR COLON basetype defVar* block END
bool defFunc() {

	printf("\n------- FIRS: defFunc -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (consume(FUNCTION)) {
		if (consume(ID)) {
			// check if the function is already defined in the current domain
			const char* name = consumed->text;
			Symbol* s = searchInCurrentDomain(name);
			if (s) tkerr("function redefinition: %s", name);

			// add the function to the current domain
			crtFn = addSymbol(name, KIND_FN);
			crtFn->args = NULL;

			addDomain();// create local domain

			crtCode = &tFunctions;	// set the current code to tFunctions
			crtVar = &tFunctions;	// set the current variable to tFunctions
			Text_clear(&tFnHeader);	// clear the function header buffer
			Text_write(&tFnHeader, "%s(", name);

			if (consume(LPAR)) {
				if (funcParams()) {
					if (consume(RPAR)) {
						if (consume(COLON)) {
							if (baseType()) {
								crtFn->type = ret.type; // set the return type of the function

								Text_write(&tFunctions, "\n%s %s){\n", cType(ret.type), tFnHeader.buf);

								while (defVar()) {}
								if (block()) {
									if (consume(END)) {
										delDomain();	// delete local domain
										crtFn = NULL;	// reset crtFn

										Text_write(&tFunctions, "}\n");	// write the footer of the function
										crtCode = &tMain;
										crtVar = &tBegin;

										printf("\n------- END: defFunc -------\n");
										return true;
									}
									else tkerr("syntax error: missing 'end' after [block]");
								}
								else tkerr("syntax error: missing [block] after [base type]");
							}
							else tkerr("syntax error: missing [base type] after ':'");
						}
						else tkerr("syntax error: missing ':' after ')'");
					}
					else tkerr("syntax error: missing ')' after [function parameters]");
				}
				else tkerr("syntax error: missing [function parameters] after '('");
			}
			else tkerr("syntax error: missing '(' after [identifier]");
		}
		else tkerr("syntax error: missing [identifier] after 'function'");
	}
	else {
		printf("\n------- END: defFunc -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}

// funcParams ::= ( funcParam (COMMA funcParam)* )?
bool funcParams() {

	printf("\n------- FIRS: funcParams -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (funcParam()) {
		if (consume(COMMA)) {

			Text_write(&tFnHeader, ",");

			if (funcParam()) {
				while (consume(COMMA)) {
					if (funcParam()) {}
					else tkerr("syntax error: missing [function parameter] after ','");
				}
			}
			else {
				tkerr("syntax error: missing [function parameter] after ','");
			}
		}
	}

	printf("\n------- END: funcParams -------\n");

	// restore iTk
	// iTk = start_iTk;
	return true;	// returns always true
}

// funcParam ::= ID COLON baseType
bool funcParam() {

	printf("\n------- FIRS: funcParam -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (consume(ID)) {
		// check if the function argument is already defined in the current domain
		const char* name = consumed->text;
		Symbol* s = searchInCurrentDomain(name);
		if (s) tkerr("domain error: function argument redefinition: %s", name);

		// add the function argument
		s = addSymbol(name, KIND_ARG);
		Symbol* sFnParam = addFnArg(crtFn, name);

		if (consume(COLON)) {
			if (baseType()) {
				// set the type of the function argument
				s->type = ret.type;
				sFnParam->type = ret.type;

				Text_write(&tFnHeader, "%s %s", cType(ret.type), name);	// write the function argument in the function header buffer

				printf("\n------- END: funcParam -------\n");
				return true;
			}
			else tkerr("syntax error: missing [base type] after ':'");
		}
		else tkerr("syntax error: missing ':' after [identifier]");
	}
	else {
		printf("\n------- END: funcParam -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}

// block ::= instr+
bool block() {

	printf("\n------- FIRS: block -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (instr()) {
		while (instr()) {}
		printf("\n------- END: block -------\n");
		return true;
	}
	else {
		printf("\n------- END: block -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}

// instr ::=  exprLogic? SEMICOLON 
//			| IF LPAR exprLogic RPAR block ( ELSE block )? END 
//			| RETURN exprLogic SEMICOLON
//			| WHILE LPAR exprLogic RPAR block END
bool instr() {
	printf("\n------- FIRS: instr -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (exprLogic()) {
		if (consume(SEMICOLON)) {

			Text_write(crtCode, ";\n");

			printf("\n------- END: instr -------\n");
			return true;
		}
		else { tkerr("syntax error: missing ';' after [expression]"); }
	}
	else if (consume(SEMICOLON)) {
		printf("\n------- END: instr -------\n");
		return true;
	}
	else if (consume(IF)) {
		if (consume(LPAR)) {
			Text_write(crtCode, "if(");
			
			if (exprLogic()) {
				if (ret.type == TYPE_STR)tkerr("type error: the if condition must have type int or real");
				if (consume(RPAR)) {
					Text_write(crtCode, "){\n");
					
					if (block()) {
						Text_write(crtCode, "}\n");
						
						if (consume(ELSE)) {
							Text_write(crtCode, "else{\n");
							
							if (block()) {
								Text_write(crtCode, "}\n");
							}
							else { tkerr("syntax error: missing [block] after 'else'"); }
						}
						if (consume(END)) {
							printf("\n------- END: instr -------\n");
							return true;
						}
						else { tkerr("syntax error: missing 'end' after [block]"); }
					}
					else { tkerr("syntax error: missing [block] after ')'"); }
				}
				else { tkerr("syntax error: missing ')' after [expression]"); }
			}
			else { tkerr("syntax error: missing [expression] after '('"); }
		}
		else { tkerr("syntax error: missing '(' after 'if'"); }
	}
	else if (consume(RETURN)) {
		Text_write(crtCode, "return ");

		if (exprLogic()) {
			if (!crtFn) tkerr("type error: return can be used only in a function");
			if (ret.type != crtFn->type) tkerr("type error: the return type must be the same as the function return type");
			
			if (consume(SEMICOLON)) {
				Text_write(crtCode, ";\n");

				printf("\n------- END: instr -------\n");
				return true;
			}
			else { tkerr("syntax error: missing ';' after [expression]"); }
		}
		else { tkerr("syntax error: missing [expression] after 'return'"); }
	}
	else if (consume(WHILE)) {
		Text_write(crtCode, "while(");

		if (consume(LPAR)) {
			if (exprLogic()) {
				if (ret.type == TYPE_STR) tkerr("type error: the while condition must have type int or real");
				if (consume(RPAR)) {
					Text_write(crtCode, "){\n");

					if (block()) {
						if (consume(END)) {
							Text_write(crtCode, "}\n");

							printf("\n------- END: instr -------\n");
							return true;
						}
						else { tkerr("syntax error: missing 'end' after [block]"); }
					}
					else { tkerr("syntax error: missing [block] after ')'"); }
				}
				else { tkerr("syntax error: missing ')' after [expression]"); }
			}
			else { tkerr("syntax error: missing [expression] after '('"); }
		}
		else { tkerr("syntax error: missing '(' after 'while'"); }
	}
	else {
		printf("\n------- END: instr -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}


// exprLogic ::= exprAssign ( (AND | OR ) exprAssign )*
bool exprLogic() {
	printf("\n------- FIRS: exprLogic -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (exprAssign()) {
		while (consume(AND) || consume(OR)) {
			int operator_code = consumed->code;

			switch (operator_code) {
				case AND:
					Text_write(crtCode, "&&");
					break;
				case OR:
					Text_write(crtCode, "||");
					break;
			}

			// check if the left operand is of type str
			Ret leftType = ret;
			if (leftType.type == TYPE_STR)tkerr("type error: the left operand of && or || cannot be of type str");

			if (exprAssign()) {
				// check if the right operand is of type str
				if (ret.type == TYPE_STR)tkerr("type error: the right operand of && or || cannot be of type str");
				setRet(TYPE_INT, false);
			}
			else {
				switch (operator_code)
				{
				case AND:
					tkerr("syntax error: missing [expression] after &&");
					break;

				case OR:
					tkerr("syntax error: missing [expression] after ||");
					break;

				default:
					tkerr("syntax error: FATAL");
					break;
				}
			}
		}
		printf("\n------- END: exprLogic -------\n");
		return true;
	}
	else {
		printf("\n------- END: exprLogic -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}



// exprAssign ::= ID ASSIGN exprComp
//					| exprComp
bool exprAssign() {
	printf("\n------- FIRS: exprAssign -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (consume(ID)) {
		const char* name = consumed->text;
		if (consume(ASSIGN)) {
			Text_write(crtCode, "%s=", name);

			if (exprComp()) {
				// check if the variable is defined
				Symbol* s = searchSymbol(name);
				if (!s) tkerr("type error: undefined symbol: %s", name);

				// check if the variable is a function
				if (s->kind == KIND_FN) tkerr("type error: a function (%s) cannot be used as a destination for assignment ", name);
				// check type compatibility
				if (s->type != ret.type) tkerr("type error: the source and destination for assignment must have the same type");

				ret.lval = false;

				printf("\n------- END: exprAssign -------\n");
				return true;
			}
			else { tkerr("syntax error: missing [expression] after '='"); }
		}
		// else { tkerr("syntax error: missing '=' after [identifier]"); }
		else {
			// restore iTk
			iTk = start_iTk;
		}

	}

	if (exprComp()) {
		printf("\n------- END: exprAssign -------\n");
		return true;
	}
	else {
		printf("\n------- END: exprAssign -------\n");
		// restore iTk
		iTk = start_iTk;
		return false;
	}


}

// exprComp ::= exprAdd ( (LESS | EQUAL) exprAdd )?
bool exprComp() {
	printf("\n------- FIRS: exprComp -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (exprAdd()) {
		if (consume(LESS) || consume(EQUAL)) {
			int operator_code = consumed->code;

			switch (operator_code) {
				case LESS:
					Text_write(crtCode, "<");
					break;

				case EQUAL:
					Text_write(crtCode, "==");
					break;
			}

			Ret leftType = ret;

			if (exprAdd()) {
				if (leftType.type != ret.type) tkerr("type error: different types for the operands of < or ==");
				setRet(TYPE_INT, false); // the result of comparation is int 0 or 1
			}
			else {
				switch (operator_code)
				{
				case LESS:
					tkerr("syntax error: missing [expression] after '<'");
					break;

				case EQUAL:
					tkerr("syntax error: missing [expression] after '=='");
					break;

				default:
					tkerr("syntax error: FATAL");
					break;
				}
			}
		}
		printf("\n------- END: exprComp -------\n");
		return true;
	}
	else {
		printf("\n------- END: exprComp -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}

// exprAdd ::= exprMul ( (ADD | SUB) exprMul )*
bool exprAdd() {
	printf("\n------- FIRS: exprAdd -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (exprMul()) {
		while (consume(ADD) || consume(SUB)) {
			int operator_code = consumed->code;

			switch (operator_code)
			{
				case ADD:
					Text_write(crtCode, "+");
					break;

				case SUB:
					Text_write(crtCode, "-");
					break;
			}

			Ret leftType = ret;
			
			if (leftType.type == TYPE_STR) tkerr("type error: the operands of + or - cannot be of type str");
			if (exprMul()) {
				if (leftType.type != ret.type) tkerr("type error: different types for the operands of + or -");
				ret.lval = false;
			}
			else {
				switch (operator_code)
				{
				case ADD:
					tkerr("syntax error: missing [expression] after '+'");
					break;

				case SUB:
					tkerr("syntax error: missing [expression] after '-'");
					break;

				default:
					tkerr("syntax error: FATAL");
					break;
				}
			}
		}
		printf("\n------- END: exprAdd -------\n");
		return true;
	}
	else {
		printf("\n------- END: exprAdd -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}

// exprMul ::= exprPrefix ( (MUL | DIV) exprPrefix )*
bool exprMul() {
	printf("\n------- FIRS: exprMul -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (exprPrefix()) {
		while (consume(MUL) || consume(DIV)) {
			int operator_code = consumed->code;

			switch (operator_code)
			{
				case MUL:
					Text_write(crtCode, "*");
					break;

				case DIV:
					Text_write(crtCode, "/");
					break;
			}

			Ret leftType = ret;
			
			if (leftType.type == TYPE_STR) tkerr("type error: the operands of * or / cannot be of type str");
			if (exprPrefix()) {
				if (leftType.type != ret.type) tkerr("type error: different types for the operands of * or /");
				ret.lval = false;
			}
			else {
				switch (operator_code)
				{
				case MUL:
					tkerr("syntax error: missing [expression] after '*'");
					break;

				case DIV:
					tkerr("syntax error: missing [expression] after '/'");
					break;

				default:
					tkerr("syntax error: FATAL");
					break;
				}
			}
		}
		printf("\n------- END: exprMul -------\n");
		return true;
	}
	else {
		printf("\n------- END: exprMul -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}

// exprPrefix ::= SUB factor
//					| NOT factor 
//					| factor
bool exprPrefix() {
	printf("\n------- FIRS: exprPrefix -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (consume(SUB)) {
		Text_write(crtCode, "-");

		if (factor()) {
			if (ret.type == TYPE_STR) tkerr("type error: the expression of unary - must be of type int or real");
			ret.lval = false;

			printf("\n------- END: exprPrefix -------\n");
			return true;
		}
		else {
			tkerr("syntax error: missing [factor] after '-'");
		}
	}
	else if (consume(NOT)) {
		Text_write(crtCode, "!");

		if (factor()) {
			if (ret.type == TYPE_STR) tkerr("type error: the expression of ! must be of type int or real");
			setRet(TYPE_INT, false);

			printf("\n------- END: exprPrefix -------\n");
			return true;
		}
		else {
			tkerr("syntax error: missing [factor] after '!'");
		}
	}
	else if (factor()) {
		printf("\n------- END: exprPrefix -------\n");
		return true;
	}
	else {
		printf("\n------- END: exprPrefix -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}


// factor ::= INT 
//			| REAL 
//			| STR 
//			| LPAR exprLogic RPAR
//			| ID (LPAR (exprLogic (COMMA exprLogic )* )? RPAR) | epsilon
bool factor() {
	printf("\n------- FIRS: factor -------\n");

	// memorize the current value of iTk
	int start_iTk = iTk;

	if (consume(INT)) {
		setRet(TYPE_INT, false);

		Text_write(crtCode, "%d", consumed->i);

		printf("\n------- END: factor -------\n");
		return true;
	}
	else if (consume(REAL)) {
		setRet(TYPE_REAL, false);

		Text_write(crtCode, "%g", consumed->r);

		printf("\n------- END: factor -------\n");
		return true;
	}
	else if (consume(STR)) {
		setRet(TYPE_STR, false);

		Text_write(crtCode, "\"%s\"", consumed->text);


		printf("\n------- END: factor -------\n");
		return true;
	}
	else if (consume(LPAR)) {
		Text_write(crtCode, "(");

		if (exprLogic()) {
			if (consume(RPAR)) {
				Text_write(crtCode, ")");

				printf("\n------- END: factor -------\n");
				return true;
			}
			else tkerr("syntax error: missing ')' after [expression]");
		}
		else tkerr("syntax error: missing [expression] after '('");
	}
	else if (consume(ID)) {
		Symbol* s = searchSymbol(consumed->text);
		if (!s) tkerr("type error: undefined symbol: %s", consumed->text);

		Text_write(crtCode, "%s", s->name);

		if (consume(LPAR)) {
			if (s->kind != KIND_FN)tkerr("type error: %s cannot be called, because it is not a function", s->name);
			Symbol* argDef = s->args;

			Text_write(crtCode, "(");

			if (exprLogic()) {
				if (!argDef) tkerr("type error: the function %s is called with too many arguments", s->name);
				if (argDef->type != ret.type) tkerr("type error: the argument type at function %s call is different from the one given at its definition", s->name);
				argDef = argDef->next;

				while (consume(COMMA)) {
					Text_write(crtCode, ",");

					if (exprLogic()) {
						if (!argDef) tkerr("type error: the function %s is called with too many arguments", s->name);
						if (argDef->type != ret.type) tkerr("type error: the argument type at function %s call is different from the one given at its definition", s->name);
						
						argDef = argDef->next;	// go to the next argument
					}
					else tkerr("syntax error: missing [expression] after ','");
				}
			}
			if (consume(RPAR)) {
				if (argDef) {
					tkerr("type error: the function %s is called with too few arguments", s->name);
					setRet(s->type, false);
				}

				Text_write(crtCode, ")");

				printf("\n------- END: factor -------\n");
				return true;
			}
		}
		else {
			if (s->kind == KIND_FN) {
				tkerr("the function %s can only be called", s->name);
				setRet(s->type, true);
			}

			printf("\n------- END: factor -------\n");
			return true;
		}
	}
	else {
		printf("\n------- END: factor -------\n");

		// restore iTk
		iTk = start_iTk;
		return false;
	}
}

