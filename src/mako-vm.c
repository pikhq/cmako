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

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "constants.h"
#include "draw.h"
#include "ui.h"

static int32_t *m;

static void push(int32_t v)
{
	m[m[DP]++] = v;
}

static void rpush(int32_t v)
{
	m[m[RP]++] = v;
}

static int32_t pop()
{
	return m[--m[DP]];
}

static int32_t rpop()
{
	return m[--m[RP]];
}

static int32_t mod(int32_t a, int32_t b)
{
	a %= b;
	return a < 0 ? a+b : a;
}

static int32_t load(int32_t addr)
{
	switch(addr) {
	case CO:
		return read_console();
	case KY:
		return read_gamepad();
	case KB:
		return read_key();
	case RN:
		return m[RN] = rand();
	case DP:
		return m[addr]-1;
	default:
		return m[addr];
	}
}

static void stor(int32_t addr, int32_t val)
{
	switch(addr) {
	case CO:
		write_console(val);
		break;
	case AU:
		write_sound(val);
		break;
	default:
		m[addr] = val;
	}
}


void run_vm() {
	int32_t o;
	int32_t a;

#define STEP				\
	if(m[PC] == -1) {		\
		finished(0);		\
		return;			\
	}				\
	o = m[m[PC]++];			\
	switch(o) {			\
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
	default: finished(1);		\
		 return;		\
	}

	STEP;

CONST:
	push(m[m[PC]++]);
	STEP;
CALL:
	rpush(m[PC]+1);
	m[PC] = m[m[PC]];
	STEP;
JUMP:
	m[PC] = m[m[PC]];
	STEP;
JUMPZ:
	m[PC] = pop()==0 ? m[m[PC]] : m[PC]+1;
	STEP;
JUMPIF:
	m[PC] = pop()!=0 ? m[m[PC]] : m[PC]+1;
	STEP;
LOAD:
	m[m[DP]-1] = load(m[m[DP]-1]);
	STEP;
STOR:
	m[DP]-=2;
	stor(m[m[DP]+1], m[m[DP]]);
	STEP;
RETURN:
	m[PC] = m[--m[RP]];
	STEP;
DROP:
	pop();
	STEP;
SWAP:
	a = m[m[DP]-1];
	m[m[DP]-1] = m[m[DP]-2];
	m[m[DP]-2] = a;
	STEP;
DUP:
	push(m[m[DP]-1]);
	STEP;
OVER:
	push(m[m[DP]-2]);
	STEP;
STR:
	rpush(pop());
	STEP;
RTS:
	push(rpop());
	STEP;
ADD:
	m[DP]--;
	m[m[DP]-1] = m[m[DP]-1] + m[m[DP]];
	STEP;
SUB:
	m[DP]--;
	m[m[DP]-1] = m[m[DP]-1] - m[m[DP]];
	STEP;
MUL:
	m[DP]--;
	m[m[DP]-1] = m[m[DP]-1] * m[m[DP]];
	STEP;
DIV:
	m[DP]--;
	m[m[DP]-1] = m[m[DP]-1] / m[m[DP]];
	STEP;
MOD:
	m[DP]--;
	m[m[DP]-1] = mod(m[m[DP]-1], m[m[DP]]);
	STEP;
AND:
	m[DP]--;
	m[m[DP]-1] = m[m[DP]-1] & m[m[DP]];
	STEP;
OR:
	m[DP]--;
	m[m[DP]-1] = m[m[DP]-1] | m[m[DP]];
	STEP;
XOR:
	m[DP]--;
	m[m[DP]-1] = m[m[DP]-1] ^ m[m[DP]];
	STEP;
NOT:
	m[m[DP]-1] = ~m[m[DP]-1];
	STEP;
SGT:
	m[DP]--;
	m[m[DP]-1] = m[m[DP]-1]>m[m[DP]] ? -1 : 0;
	STEP;
SLT:
	m[DP]--;
	m[m[DP]-1] = m[m[DP]-1]<m[m[DP]] ? -1 : 0;
	STEP;
NEXT:
	m[PC] = --m[m[RP]-1]<0 ? m[PC]+1 : m[m[PC]];
	STEP;
SYNC:
	draw(m);
	return;
}

void init_vm(int32_t *mem)
{
	m = mem;
	srand(m[RN]);
}
