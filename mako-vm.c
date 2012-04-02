#ifndef USE_GL
#define USE_GL 0
#endif

#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <SDL.h>
#include <SDL_video.h>

#if USE_GL == 1
#include <GL/gl.h>
#endif

#include "constants.h"

extern char *argv0;

#define SND_BUF_SIZE 1024

static int32_t *m;
static int32_t key_buf[1024];
static uint8_t snd_buf[SND_BUF_SIZE];
static int key_buf_r, key_buf_w;
static int snd_buf_r = 1, snd_buf_w = 0;
enum ring_buf { BUF_READ, BUF_WRITE };
static enum ring_buf key_buf_op = BUF_READ;
static int sound_playing;
static uint32_t execution_start;
static int stored;

static void draw(SDL_Surface*);

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
	if(addr == CO)
		return (int32_t)getchar();
	else if(addr == KB) {
		if(key_buf_r == key_buf_w && key_buf_op == BUF_READ)
			return -1;

		key_buf_r++;
		key_buf_r %= 1024;
		key_buf_op = BUF_READ;
		return key_buf[key_buf_r];
	} else if(addr == RN)
		return rand();
	else
		return m[addr];
}

static void stor(int32_t addr, int32_t val)
{
	if(addr == CO)
		putchar(val);
	else if(addr == AU) {
		if(!sound_playing) {
			sound_playing = 1;
			SDL_PauseAudio(0);
		}
		SDL_LockAudio();
		while((snd_buf_w+1) % SND_BUF_SIZE == snd_buf_r) {
			SDL_UnlockAudio();
			SDL_Delay(0);
			SDL_LockAudio();
		}
		snd_buf_w++;
		snd_buf_w %= SND_BUF_SIZE;
		snd_buf[snd_buf_w] = val;
		SDL_UnlockAudio();
	} else {
		stored = 1;
		m[addr] = val;
	}
}

#if USE_GL
GLuint tex;
#endif

static void sync() {
	static SDL_Surface *scr = NULL;

	if(!scr) {
#if USE_GL
		scr = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, 320, 0, 240, -1, 0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &tex);
#else
		scr = SDL_SetVideoMode(320, 240, 32, SDL_SWSURFACE);
#endif
		if(!scr) {
			fprintf(stderr, "%s: %s\n", argv0, SDL_GetError());
			exit(1);
		}
	}

	SDL_Event event;		

	while(SDL_PollEvent(&event)) {
		switch(event.type) {
		case SDL_KEYDOWN:
			if(event.key.keysym.unicode) {
				key_buf_w++;
				key_buf_w %= 1024;
				key_buf_op = BUF_WRITE;
				if(event.key.keysym.unicode == '\r')
					key_buf[key_buf_w] = '\n';
				else
					key_buf[key_buf_w] = event.key.keysym.unicode;
			}
			// !!! Intentional fallthrough
		case SDL_KEYUP:
			switch(event.key.keysym.sym) {
#define SET_KEY(sdl, mako) \
	case sdl: \
		if (event.type == SDL_KEYDOWN) \
			m[KY] |= mako; \
		else \
			m[KY] &= ~mako; \
		break;
			SET_KEY(SDLK_LEFT, KEY_LF);
			SET_KEY(SDLK_RIGHT, KEY_RT);
			SET_KEY(SDLK_UP, KEY_UP);
			SET_KEY(SDLK_DOWN, KEY_DN);
			SET_KEY(SDLK_RETURN, KEY_A);
			SET_KEY(SDLK_SPACE, KEY_A);
			SET_KEY(SDLK_z, KEY_A);
			SET_KEY(SDLK_x, KEY_B);
			SET_KEY(SDLK_LSHIFT, KEY_B);
			SET_KEY(SDLK_RSHIFT, KEY_B);
#undef SET_KEY
			}
			break;
		case SDL_QUIT:
			exit(0);
		}
	}

	draw(scr);

	uint32_t total = SDL_GetTicks() - execution_start;
	
	if(total < 1000/60)
		SDL_Delay(1000/60 - total);

	execution_start = SDL_GetTicks();
}


static void tick() {
	if(m[PC] == -1)
		exit(0);

	int32_t o = m[m[PC]++];
	int32_t a;

	switch(o) {
	case OP_CONST:
		push(m[m[PC]++]);
		break;
	case OP_CALL:
		rpush(m[PC]+1);
		m[PC] = m[m[PC]];
		break;
	case OP_JUMP:
		m[PC] = m[m[PC]];
		break;
	case OP_JUMPZ:
		m[PC] = pop()==0 ? m[m[PC]] : m[PC]+1;
		break;
	case OP_JUMPIF:
		m[PC] = pop()!=0 ? m[m[PC]] : m[PC]+1;
		break;
	case OP_LOAD:
		push(load(pop()));
		break;
	case OP_STOR:
		m[DP]-=2;
		stor(m[m[DP]+1], m[m[DP]]);
		break;
	case OP_RETURN:
		m[PC] = rpop();
		break;
	case OP_DROP:
		pop();
		break;
	case OP_SWAP:
		a = m[m[DP]-1];
		m[m[DP]-1] = m[m[DP]-2];
		m[m[DP]-2] = a;
		break;
	case OP_DUP:
		push(m[m[DP]-1]);
		break;
	case OP_OVER:
		push(m[m[DP]-2]);
		break;
	case OP_STR:
		rpush(pop());
		break;
	case OP_RTS:
		push(rpop());
		break;
	case OP_ADD:
		m[DP]--;
		m[m[DP]-1] = m[m[DP]-1] + m[m[DP]];
		break;
	case OP_SUB:
		m[DP]--;
		m[m[DP]-1] = m[m[DP]-1] - m[m[DP]];
		break;
	case OP_MUL:
		m[DP]--;
		m[m[DP]-1] = m[m[DP]-1] * m[m[DP]];
		break;
	case OP_DIV:
		m[DP]--;
		m[m[DP]-1] = m[m[DP]-1] / m[m[DP]];
		break;
	case OP_MOD:
		m[DP]--;
		m[m[DP]-1] = mod(m[m[DP]-1], m[m[DP]]);
		break;
	case OP_AND:
		m[DP]--;
		m[m[DP]-1] = m[m[DP]-1] & m[m[DP]];
		break;
	case OP_OR:
		m[DP]--;
		m[m[DP]-1] = m[m[DP]-1] | m[m[DP]];
		break;
	case OP_XOR:
		m[DP]--;
		m[m[DP]-1] = m[m[DP]-1] ^ m[m[DP]];
		break;
	case OP_NOT:
		m[m[DP]-1] = ~m[m[DP]-1];
		break;
	case OP_SGT:
		m[DP]--;
		m[m[DP]-1] = m[m[DP]-1]>m[m[DP]] ? -1 : 0;
		break;
	case OP_SLT:
		m[DP]--;
		m[m[DP]-1] = m[m[DP]-1]<m[m[DP]] ? -1 : 0;
		break;
	case OP_NEXT:
		m[PC] = --m[m[RP]-1]<0 ? m[PC]+1 : m[m[PC]];
		break;
	case OP_SYNC:
		sync();
		break;
	}
}

#if USE_GL
static uint32_t buf[240][320];
#endif

static void draw_pixel(SDL_Surface *scr, uint32_t x, uint32_t y, uint32_t col)
{
	if((col & 0xFF000000) != 0xFF000000) return;
	if(x < 0 || x >= 320 || y < 0 || y >= 240) return;
#if USE_GL
	buf[y][x] = col;
#else
	uint32_t *buf = scr->pixels;
	buf[x + y*(scr->pitch/4)] = col;
#endif
}

static void draw_tile(SDL_Surface *scr, int32_t tile, int px, int py)
{
	if(tile < 0) return;
	tile &= ~GRID_Z_MASK;

	uint32_t i = m[GT] + tile * 64;

	for(int y = 0; y < 8; y++)
		for(int x = 0; x < 8; x++)
			draw_pixel(scr, x + px, y + py, m[i++]);
}

static void draw_sprite(SDL_Surface *scr, int32_t tile, int32_t status, int px, int py)
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
			draw_pixel(scr, x+px, y+py, m[i++]);
}

static void draw_grid(SDL_Surface *scr, int zbit)
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
			draw_tile(scr, m[i++], x*8 - m[SX], y*8 - m[SY]);
		}
		i += m[GS];
	}
}

static void draw(SDL_Surface *scr)
{
	if(stored) {
#if USE_GL
		for(int i = 0; i < 320; i++)
			for(int j = 0; j < 240; j++)
				buf[j][i] = m[CL];
#else
		if(SDL_MUSTLOCK(scr))
			while(SDL_LockSurface(scr) != 0) SDL_Delay(10);

		SDL_FillRect(scr, NULL, m[CL]);
#endif

		draw_grid(scr, 0);

		for(int spr = 0; spr < 1024; spr+=4) {
			int32_t status = m[m[SP] + spr];
			int32_t tile = m[m[SP] + spr + 1];
			int32_t px = m[m[SP] + spr + 2];
			int32_t py = m[m[SP] + spr + 3];
			draw_sprite(scr, tile, status, px - m[SX], py - m[SY]);
		}

		draw_grid(scr, 1);

#if USE_GL
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, 320, 240, 0, GL_BGRA, GL_UNSIGNED_BYTE, buf);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(0, 240);
			glTexCoord2f(1, 0); glVertex2f(320, 240);
			glTexCoord2f(1, 1); glVertex2f(320, 0);
			glTexCoord2f(0, 1); glVertex2f(0, 0);
		glEnd();
		glFlush();
	}
	SDL_GL_SwapBuffers();
#else
		if(SDL_MUSTLOCK(scr))
			SDL_UnlockSurface(scr);
	}

	SDL_UpdateRect(scr, 0, 0, 0, 0);
#endif
	stored=0;
}

static void snd_callback(void *userdata, uint8_t *stream, int len)
{
	while (snd_buf_r == snd_buf_w);

	for(int i = 0; i < len && snd_buf_w != (snd_buf_r+1) % SND_BUF_SIZE; i++) {
		snd_buf_r++;
		snd_buf_r %= SND_BUF_SIZE;
		stream[i] = snd_buf[snd_buf_r];
	}
}

void run_vm(int32_t *mem, char *name)
{
	m = mem;
	
	srand(time(0));

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
	signal(SIGINT, SIG_DFL);
	atexit(SDL_Quit);
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	if(name)
		SDL_WM_SetCaption(name, NULL);

	SDL_AudioSpec desired = {.freq = 8000, .format = AUDIO_U8, .channels = 1, .callback = snd_callback, .samples=128};

	if(SDL_OpenAudio(&desired, NULL)) {
		fprintf(stderr, "%s: %s", argv0, SDL_GetError());
		exit(1);
	}

	execution_start = SDL_GetTicks();

	while(1)
		tick();
}
