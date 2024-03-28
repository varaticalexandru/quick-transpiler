#pragma once

#include <stdbool.h>


void tkerr(const char* fmt, ...);
bool consume(int code);
bool program();
bool defVar();
bool defFunc();
bool block();
bool baseType();
bool funcParams();
bool funcParam();
bool block();
bool instr();
bool exprLogic();
bool exprAssign();
bool exprComp();
bool exprAdd();
bool exprMul();
bool exprPrefix();
bool factor();

// parse the extracted tokens
void parse();