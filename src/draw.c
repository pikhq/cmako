#include <stdint.h>
#include <string.h>

#include "constants.h"
#include "draw.h"

uint32_t framebuf[240][320];

static void unsafe_draw_pixel(uint32_t x, uint32_t y, uint32_t col)
{
	if((col & 0xFF000000) ^ 0xFF000000) return;
	framebuf[y][x] = col;
}

static void draw_pixel(uint32_t x, uint32_t y, uint32_t col)
{
	if((col & 0xFF000000) ^ 0xFF000000) return;
	if(x < 0 || x >= 320 || y < 0 || y >= 240) return;

	framebuf[y][x] = col;
}

static void draw_tile(int32_t *m, int32_t tile, int32_t px, int32_t py)
{
	if(tile < 0) return;
	tile &= ~GRID_Z_MASK;

	uint32_t i = m[GT] + tile * 64;
	int x_pre = 0;
	int x_post = 0;
	int y_pre = 0;
	int x_lim = px+8;
	int y_lim = py+8;
	int x_str = px;
	int y_str = py;

	if(x_lim > 320) {
		x_lim = 320;
		x_post = 8 + px - 320;
	}
	if(8 + py > 240) {
		y_lim = 240;
	}
	if(px < 0) {
		x_str = 0;
		x_pre = -px;
	}
	if(py < 0) {
		y_str = 0;
		y_pre = -py * 8;
	}

	i += y_pre;
	for(int y = y_str; y < y_lim; y++) {
		i += x_pre;
		for(int x = x_str; x < x_lim; x++)
			unsafe_draw_pixel(x, y, m[i++]);
		i += x_post;
	}
}

static void draw_sprite(int32_t *m, int32_t tile, int32_t status, int32_t px, int32_t py)
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
		for(int x = x0; x != x1; x+=xd)
			draw_pixel(x + px, y + py, m[i++]);
}

static void draw_grid(int32_t *m, int zbit)
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
			draw_tile(m, m[i++], x*8 - m[SX], y*8 - m[SY]);
		}
		i += m[GS];
	}
}

void draw(int32_t *m)
{
	for(int x = 0; x < 320; x++)
		framebuf[0][x] = m[CL];

	for(int y = 1; y < 240; y++)
		memcpy(framebuf[y], framebuf[0], sizeof(framebuf[0]));

	draw_grid(m, 0);

	for(int spr = 0; spr < 1024; spr+=4) {
		int32_t status = m[m[SP] + spr];
		int32_t tile = m[m[SP] + spr + 1];
		int32_t px = m[m[SP] + spr + 2];
		int32_t py = m[m[SP] + spr + 3];
		draw_sprite(m, tile, status, px - m[SX], py - m[SY]);
	}

	draw_grid(m, 1);
}
