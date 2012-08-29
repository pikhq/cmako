/* 
 * Copyright (c) 2012 cmako contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "constants.h"
#include "draw.h"
#include "ui.h"

#if HAVE_COMPUTED_GOTO
typedef void *instr;
#else
typedef int32_t instr;
#endif

static int32_t *m;
static instr *code;
static size_t s;

#define push(v) { *dp++ = v; }
#define rpush(v) { *rp++ = v; }
#define top() dp[-1]
#define rtop() rp[-1]
#define pop() *--dp
#define rpop() *--rp

static int32_t mod(int32_t a, int32_t b)
{
	a %= b;
	return a < 0 ? a+b : a;
}

void run_vm() {
	int32_t a, b;
	int32_t *dp = m+m[DP];
	int32_t *rp = m+m[RP];

#if HAVE_COMPUTED_GOTO
	static const instr jmp[] = {
		[OP_CONST] = &&CONST, [OP_CALL] = &&CALL,
		[OP_JUMP] = &&JUMP, [OP_JUMPZ] = &&JUMPZ,
		[OP_JUMPIF] = &&JUMPIF, [OP_LOAD] = &&LOAD,
		[OP_STOR] = &&STOR, [OP_RETURN] = &&RETURN,
		[OP_DROP] = &&DROP, [OP_SWAP] = &&SWAP,
		[OP_DUP] = &&DUP, [OP_OVER] = &&OVER,
		[OP_STR] = &&STR, [OP_RTS] = &&RTS,
		[OP_ADD] = &&ADD, [OP_SUB] = &&SUB,
		[OP_MUL] = &&MUL, [OP_DIV] = &&DIV,
		[OP_MOD] = &&MOD, [OP_AND] = &&AND,
		[OP_OR] = &&OR, [OP_XOR] = &&XOR,
		[OP_NOT] = &&NOT, [OP_SGT] = &&SGT,
		[OP_SLT] = &&SLT, [OP_NEXT] = &&NEXT,
		[OP_SYNC] = &&SYNC, [OP_LOOKUP] = &&LOOKUP,
		[OP_EXIT] = &&EXIT, [OP_HCF] = &&HCF,
		[5] = &&HCF, [6] = &&HCF, [7] = &&HCF,
		[8] = &&HCF, [9] = &&HCF
	};
#define STEP goto **pc;
#else
	static const instr jmp[] = {
		[OP_CONST] = OP_CONST, [OP_CALL] = OP_CALL,
		[OP_JUMP] = OP_JUMP, [OP_JUMPZ] = OP_JUMPZ,
		[OP_JUMPIF] = OP_JUMPIF, [OP_LOAD] = OP_LOAD,
		[OP_STOR] = OP_STOR, [OP_RETURN] = OP_RETURN,
		[OP_DROP] = OP_DROP, [OP_SWAP] = OP_SWAP,
		[OP_DUP] = OP_DUP, [OP_OVER] = OP_OVER,
		[OP_STR] = OP_STR, [OP_RTS] = OP_RTS,
		[OP_ADD] = OP_ADD, [OP_SUB] = OP_SUB,
		[OP_MUL] = OP_MUL, [OP_DIV] = OP_DIV,
		[OP_MOD] = OP_MOD, [OP_AND] = OP_AND,
		[OP_OR] = OP_OR, [OP_XOR] = OP_XOR,
		[OP_NOT] = OP_NOT, [OP_SGT] = OP_SGT,
		[OP_SLT] = OP_SLT, [OP_NEXT] = OP_NEXT,
		[OP_SYNC] = OP_SYNC, [OP_LOOKUP] = OP_LOOKUP,
		[OP_EXIT] = OP_EXIT, [OP_HCF] = OP_HCF,
		[5] = OP_HCF, [6] = OP_HCF, [7] = OP_HCF,
		[8] = OP_HCF, [9] = OP_HCF
	};
#define STEP				\
	switch(*pc) {			\
	case OP_CONST: goto CONST;	\
	case OP_CALL: goto CALL;	\
	case OP_JUMP: goto JUMP;	\
	case OP_JUMPZ: goto JUMPZ;	\
	case OP_JUMPIF: goto JUMPIF;	\
	case OP_LOAD: goto LOAD;	\
	case OP_STOR: goto STOR;	\
	case OP_RETURN: goto RETURN;	\
	case OP_DROP: goto DROP;	\
	case OP_SWAP: goto SWAP;	\
	case OP_DUP: goto DUP;		\
	case OP_OVER: goto OVER;	\
	case OP_STR: goto STR;		\
	case OP_RTS: goto RTS;		\
	case OP_ADD: goto ADD;		\
	case OP_SUB: goto SUB;		\
	case OP_MUL: goto MUL;		\
	case OP_DIV: goto DIV;		\
	case OP_MOD: goto MOD;		\
	case OP_AND: goto AND;		\
	case OP_OR: goto OR;		\
	case OP_XOR: goto XOR;		\
	case OP_NOT: goto NOT;		\
	case OP_SGT: goto SGT;		\
	case OP_SLT: goto SLT;		\
	case OP_NEXT: goto NEXT;	\
	case OP_SYNC: goto SYNC;	\
	case OP_LOOKUP: goto LOOKUP;	\
	case OP_EXIT: goto EXIT;	\
	case OP_HCF: goto HCF;		\
	}
#endif

	if(!code) {
		code = (instr*)malloc((s + 2) * sizeof(instr)) + 1;
		if(!code) {
			finished(1);
			return;
		}
		for(size_t i = 0; i < s; i++) {
			code[i] = jmp[OP_LOOKUP];
		}
		code[-1] = jmp[OP_EXIT];
		code[s] = jmp[OP_HCF];
	}

	instr *pc = code+m[PC];
	STEP;

CONST:
	a = ((uintptr_t)pc-(uintptr_t)code)/sizeof(instr);
	push(m[a+1]);
	pc+=2;
	STEP;
CALL:
	a = ((uintptr_t)pc-(uintptr_t)code)/sizeof(instr);
	rpush(a + 2);
	pc = code + m[a+1];
	STEP;
JUMP:
	a = ((uintptr_t)pc-(uintptr_t)code)/sizeof(instr);
	pc = code + m[a+1];
	STEP;
JUMPZ:
	a = ((uintptr_t)pc-(uintptr_t)code)/sizeof(instr);
	pc = pop()==0 ? code + m[a + 1] : pc+2;
	STEP;
JUMPIF:
	a = ((uintptr_t)pc-(uintptr_t)code)/sizeof(instr);
	pc = pop()!=0 ? code + m[a + 1] : pc+2;
	STEP;
LOAD:
	pc++;
	switch(top()) {
	case CO:
		top() = read_console(); STEP;
	case KY:
		top() = read_gamepad(); STEP;
	case KB:
		top() = read_key(); STEP;
	case RN:
		top() = rand(); m[RN] = top(); STEP;
	case PC:
		top() = ((uintptr_t)pc-(uintptr_t)code)/sizeof(instr);
		STEP;
	case DP:
		top() = ((uintptr_t)dp-(uintptr_t)m)/sizeof(int32_t) - 1;
		STEP;
	case RP:
		top() = ((uintptr_t)rp-(uintptr_t)m)/sizeof(int32_t);
		STEP;
	default:
		top() = m[top()];
		STEP;
	}
STOR:
	a = pop(); b = pop();
	pc++;
	code[a] = jmp[OP_LOOKUP];
	switch(a) {
	case CO:
		write_console(b); STEP;
	case AU:
		write_sound(b); STEP;
	case PC:
		pc = code + b;
		STEP;
	case DP:
		dp = m + b;
		STEP;
	case RP:
		rp = m + b;
		STEP;
	default:
		m[a] = b;
		STEP;
	}
RETURN:
	pc = code + rpop();
	STEP;
DROP:
	pc++;
	pop();
	STEP;
SWAP:
	pc++;
	a = pop(); b = pop();
	push(a); push(b);
	STEP;
DUP:
	pc++;
	push(top());
	STEP;
OVER:
	pc++;
	push(dp[-2]);
	STEP;
STR:
	pc++;
	rpush(pop());
	STEP;
RTS:
	pc++;
	push(rpop());
	STEP;
ADD:
	pc++;
	a=pop(); b=pop();
	push(b+a);
	STEP;
SUB:
	pc++;
	a=pop(); b=pop();
	push(b-a);
	STEP;
MUL:
	pc++;
	a=pop(); b=pop();
	push(b*a);
	STEP;
DIV:
	pc++;
	a=pop(); b=pop();
	push(b/a);
	STEP;
MOD:
	pc++;
	a=pop(); b=pop();
	push(mod(b,a));
	STEP;
AND:
	pc++;
	a=pop(); b=pop();
	push(b&a);
	STEP;
OR:
	pc++;
	a=pop(); b=pop();
	push(b|a);
	STEP;
XOR:
	pc++;
	a=pop(); b=pop();
	push(b^a);
	STEP;
NOT:
	pc++;
	top() = ~top();
	STEP;
SGT:
	pc++;
	a=pop(); b=pop();
	push(b>a ? -1 : 0);
	STEP;
SLT:
	pc++;
	a=pop(); b=pop();
	push(b<a ? -1 : 0);
	STEP;
NEXT:
	a = ((uintptr_t)pc - (uintptr_t)code)/sizeof(instr);
	pc = --rp[-1]<0 ? pc + 2 : code + m[a + 1];
	STEP;
LOOKUP:
	a = ((uintptr_t)pc-(uintptr_t)code)/sizeof(instr);
	if(m[a] <= OP_NEXT)
		*pc = jmp[m[a]];
	else {
		*pc = jmp[OP_HCF];
	}
	STEP;
EXIT:
	finished(0);
	return;
HCF:
	finished(1);
	return;
SYNC:
	m[PC] = ((uintptr_t)pc - (uintptr_t)code)/sizeof(instr)+1;
	m[DP] = ((uintptr_t)dp - (uintptr_t)m)/sizeof(int32_t);
	m[RP] = ((uintptr_t)rp - (uintptr_t)m)/sizeof(int32_t);
	draw(m);
	return;
}

void init_vm(int32_t *mem, size_t size)
{
	if(code) {
		free(code-1);
		code = NULL;
	}
	m = mem;
	s = size;
	srand(m[RN]);
}
