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

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 1))
#define PREFETCH(...) __builtin_prefetch(__VA_ARGS__)
#else
#define PREFETCH(...)
#endif

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "constants.h"
#include "ui.h"

static int32_t *m;
static int32_t grid_end;
static int32_t sprite_end;
static int redraw_grid = 1;
static int redraw_sprite = 1;

uint32_t framebuf[240][320];

static void draw();

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

static void get_grid_end()
{
	grid_end = 0;
	for(int i = 0; i < 41*31; i++) {
		int32_t tile = m[m[GP]+i];
		if(tile < 0) continue;
		int32_t cur_end = m[GT] + tile*8 + 8;
		if(cur_end > grid_end)
			cur_end = grid_end;
	}
}

static void get_sprite_end()
{
	sprite_end = 0;
	for(int i = 0; i < 1024; i+=4) {
		int32_t status = m[m[SP]+i];
		int32_t tile = m[m[SP]+i+1];

		if(tile < 0) continue;

		int w = (((status & 0x0F00) >> 8) + 1) * 8;
		int h = (((status & 0xF000) >> 12) + 1) * 8;

		int32_t cur_end = m[ST] + tile*w*h + w*h;
		if(cur_end > sprite_end)
			sprite_end = cur_end;
	}

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

	case SX: case SY:
		redraw_grid = 1;
		redraw_sprite = 1;
		m[addr] = val;
		break;

	case GP:
		get_grid_end();
	case CL: case GT:
		redraw_grid = 1;
		m[addr] = val;
		break;

	case SP:
		get_sprite_end();
	case ST:
		redraw_sprite = 1;
		m[addr] = val;
		break;

	default:
		if(addr >= m[GP] && addr <= m[GP] + 41*31) {
			get_grid_end();
			redraw_grid = 1;
		} else if(addr >= m[ST] && addr <= sprite_end) {
			redraw_sprite = 1;
		} else if(addr >= m[GT] && addr <= grid_end) {
			redraw_grid = 1;
		}

		m[addr] = val;
	}
}


static void draw_pixel(uint32_t x, uint32_t y, uint32_t col)
{
	if((col & 0xFF000000) ^ 0xFF000000) return;
	if(x < 0 || x >= 320 || y < 0 || y >= 240) return;

	framebuf[y][x] = col;
}

static void unsafe_draw_pixel(int32_t x, int32_t y, uint32_t col)
{
	if((col & 0xFF000000) ^ 0xFF000000) return;

	framebuf[y][x] = col;
}

static void draw_tile(int32_t tile, int32_t px, int32_t py)
{
	if(tile < 0) return;
	tile &= ~GRID_Z_MASK;

	uint32_t i = m[GT] + tile * 64;

	int minx = 0;
	int miny = 0;
	int maxx = 8;
	int maxy = 8;

	if(px < 0)
		minx = -px;
	if(py < 0)
		miny = -py;

	if(px >= 312)
		maxx = 320-px;
	if(py >= 232)
		maxy = 240-py;

	i += 8 * miny;


	for(int y = miny; y < maxy; y++) {
		i += minx;
		for(int x = minx; x < maxx; x++) {
			unsafe_draw_pixel(x + px, y + py, m[i++]);
			PREFETCH(&framebuf[y+py+1][x+px+1], 1);
		}
		i += (8-maxx);
		PREFETCH(&m[i], 0, 0);
	}
}

static void draw_sprite(int32_t tile, int32_t status, int32_t px, int32_t py)
{
	if (status % 2 == 0) return;
	int w = (((status & 0x0F00) >> 8) + 1) * 8;
	int h = (((status & 0xF000) >> 12) + 1) * 8;

	int xd = 1;
	int yd = 1;
	int x0 = 0;
	int y0 = 0;
	int x1 = w;
	int y1 = h;

	if ((status & H_MIRROR_MASK) != 0) {
		xd = -1;
		x0 = w - 1;
		x1 = -1;
	}
	if((status & V_MIRROR_MASK) != 0) {
		yd = -1;
		y0 = h - 1;
		y1 = -1;
	}

	int i = m[ST] + (tile * w * h);
	for(int y = y0; y != y1; y+=yd)
		for(int x = x0; x != x1; x+=xd) {
			draw_pixel(x + px, y + py, m[i++]);
			PREFETCH(&m[i]);
		}
}

static void draw_grid(int zbit)
{
	int i = m[GP];
	for(int y = 0; y < 31; y++) {
		for(int x = 0; x < 41; x++) {
			if(!zbit && (m[i] & GRID_Z_MASK) != 0) {
				i++;
				continue;
			}
			if(zbit && (m[i] & GRID_Z_MASK) == 0) {
				i++;
				continue;
			}
			draw_tile(m[i++], x*8 - m[SX], y*8 - m[SY]);
		}
		i += m[GS];
	}
}

static void draw()
{
	if(redraw_grid || redraw_sprite) {
		for(int y = 0; y < 240; y++)
			for(int x = 0; x < 320; x++)
				framebuf[y][x] = m[CL];

		draw_grid(0);

		for(int spr = 0; spr < 1024; spr+=4) {
			int32_t status = m[m[SP] + spr];
			int32_t tile = m[m[SP] + spr + 1];
			int32_t px = m[m[SP] + spr + 2];
			int32_t py = m[m[SP] + spr + 3];
			draw_sprite(tile, status, px - m[SX], py - m[SY]);
		}

		draw_grid(1);

	}
	redraw_grid=0;
	redraw_sprite=0;
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
	draw();
	return;
}

void init_vm(int32_t *mem)
{
	m = mem;
	srand(time(0));

	get_grid_end();
	get_sprite_end();
}
