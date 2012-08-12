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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#include "SDL_framerate.h"

#include "ui.h"
#include "mako-vm.h"
#include "constants.h"

static uint32_t buttons;

static uint32_t key_buf[1024];
static int key_buf_r, key_buf_w;
static int key_buf_wrote;

static int sound_playing;
static uint8_t snd_buf[1024];
static int snd_buf_r = 1, snd_buf_w = 0;

static int inited_sdl = 0;

static FPSmanager fps;

static void init_sdl(void);

int32_t read_gamepad(void)
{
	return buttons;
}

int32_t read_key(void)
{
	if(key_buf_w == key_buf_r && key_buf_wrote == 0) {
		return -1;
	} else {
		key_buf_r++;
		key_buf_r %= 1024;
		key_buf_wrote = 0;
		return key_buf[key_buf_r];
	}
}

int32_t read_console(void)
{
	return getchar();
}

void write_console(int32_t c)
{
	putchar(c);
}

void write_sound(uint8_t sample)
{
	if(sound_playing == -1)
		return;

	if(!inited_sdl)
		init_sdl();

	if(!sound_playing) {
		sound_playing = 1;
		SDL_PauseAudio(0);
	}
	SDL_LockAudio();
	while((snd_buf_w+1) % 1024 == snd_buf_r) {
		SDL_UnlockAudio();
		SDL_Delay(0);
		SDL_LockAudio();
	}
	snd_buf_w++;
	snd_buf_w %= 1024;
	snd_buf[snd_buf_w] = sample;
	SDL_UnlockAudio();
}

void finished(int code)
{
	exit(code);
}

static int32_t *read_file(const char *file, size_t *size)
{
	int pos = 0;

	FILE *f = fopen(file, "rb");
	if(!f)
		return NULL;
	
	int32_t *mem = calloc(1024, sizeof *mem);
	int alloc_size = 1024;
	if(!mem)
		return NULL;

	while(!feof(f)) {
		if(pos == alloc_size) {
			alloc_size *= 2;
			mem = realloc(mem, alloc_size * sizeof *mem);
			if(!mem)
				return NULL;
		}

		uint8_t buf[4];

		int n = fread(buf, sizeof *buf, 4, f);
		if(ferror(f))
			return NULL;
		if(n == 0) break;
		if(n != 4)
			return NULL;
		mem[pos] = (int32_t)buf[0] << 24 | (int32_t)buf[1] << 16 | (int32_t)buf[2] << 8 | (int32_t)buf[3];
		pos++;
	}

	if(fclose(f) != 0)
		return NULL;

	memset(mem + pos, 0, (alloc_size - pos) * sizeof *mem);

	if(size)
		*size = pos;

	return mem;
}

static void snd_callback(void *userdata, uint8_t *stream, int len)
{
	for(int i = 0; i < len && snd_buf_w != (snd_buf_r+1) % 1024; i++) {
		snd_buf_r++;
		snd_buf_r %= 1024;
		stream[i] = snd_buf[snd_buf_r];
	}
}

static void init_sdl()
{
	SDL_putenv("SDL_NOMOUSE=1");
	SDL_putenv("SDL_VIDEO_CENTERED=1");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
	atexit(SDL_Quit);
	signal(SIGINT, SIG_DFL);
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	SDL_AudioSpec desired = {.freq = 8000, .format = AUDIO_U8, .channels = 1, .callback = snd_callback, .samples=128};
	if(SDL_OpenAudio(&desired, NULL)) {
		fprintf(stderr, "%s\n", SDL_GetError());
		sound_playing = -1;
	}

	SDL_initFramerate(&fps);
	SDL_setFramerate(&fps, 60);

	inited_sdl = 1;
}

static void button_change(int up, SDLKey keysym)
{
	const uint32_t key_map[SDLK_LAST] = {
		[SDLK_LEFT] = KEY_LF,
		[SDLK_RIGHT] = KEY_RT,
		[SDLK_UP] = KEY_UP,
		[SDLK_DOWN] = KEY_DN,
		[SDLK_RETURN] = KEY_A,
		[SDLK_SPACE] = KEY_A,
		[SDLK_z] = KEY_A,
		[SDLK_x] = KEY_B,
		[SDLK_LSHIFT] = KEY_B,
		[SDLK_RSHIFT] = KEY_A
	};

	/* Don't invoke undefined behavior if they've added keysyms
	 * since this was compiled.
	 */
	if(keysym > SDLK_LAST)
		return;
	
	if(!key_map[keysym])
		return;

	if(up) {
		buttons &= ~key_map[keysym];
	} else {
		buttons |= key_map[keysym];
	}
}

static void push_keybuf(uint32_t c)
{
	key_buf_w++;
	key_buf_w %= 1024;
	key_buf[key_buf_w] = c;
	key_buf_wrote = 1;
}

static void process_events()
{
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
		case SDL_KEYDOWN:
			if(event.key.keysym.unicode == '\r')
				push_keybuf('\n');
			else if(event.key.keysym.unicode)
				push_keybuf(event.key.keysym.unicode);

			if(event.key.keysym.sym == SDLK_ESCAPE) {
				SDL_Quit();
				exit(0);
			}

			button_change(0, event.key.keysym.sym);
			break;
		case SDL_KEYUP:
			button_change(1, event.key.keysym.sym);
			break;
		case SDL_VIDEORESIZE:
			if(!SDL_SetVideoMode(event.resize.w, event.resize.h, 0,
					SDL_HWSURFACE | SDL_ANYFORMAT | SDL_DOUBLEBUF | SDL_RESIZABLE)) {
				fprintf(stderr, "%s.\n", SDL_GetError());
				exit(1);
			}
			break;
		case SDL_QUIT:
			SDL_Quit();
			exit(0);
		}
	}

}

static void render_screen()
{
	static int screen_inited;

	static SDL_Surface *real_screen;
	static SDL_Surface *mako_screen;

	if(!inited_sdl)
		init_sdl();

	if(!screen_inited) {
		const SDL_VideoInfo *info = SDL_GetVideoInfo();

		if(info->wm_available) {
			if(!SDL_SetVideoMode(320, 240, 0,
						SDL_HWSURFACE | SDL_ANYFORMAT | SDL_DOUBLEBUF | SDL_RESIZABLE)) {
				fprintf(stderr, "%s.\n", SDL_GetError());
				exit(1);
			}
		} else {
			if(!SDL_SetVideoMode(info->current_w, info->current_h, 0,
						SDL_HWSURFACE | SDL_ANYFORMAT | SDL_DOUBLEBUF | SDL_RESIZABLE)) {
				fprintf(stderr, "%s.\n", SDL_GetError());
				exit(1);
			}
			SDL_ShowCursor(SDL_DISABLE);
		}

		real_screen = SDL_GetVideoSurface();
		mako_screen = SDL_CreateRGBSurfaceFrom(framebuf, 320, 240, 32,
				320*sizeof(uint32_t), 0xFF0000, 0xFF00, 0xFF, 0);

		screen_inited = 1;
	}

	if(real_screen->h == 240 && real_screen->w == 320)
		SDL_BlitSurface(mako_screen, NULL, real_screen, NULL);
	else {
		int desired_h = real_screen->w * 3 / 4;
		if(desired_h > real_screen->h)
			desired_h = real_screen->h;
		int desired_w = desired_h * 4 / 3;

		SDL_Rect rect = {
			.x = (real_screen->w - desired_w)/2,
			.y = (real_screen->h - desired_h)/2,
			.w = desired_w,
			.h = desired_h
		};

		SDL_SoftStretch(mako_screen, NULL, real_screen, &rect);
	}

	SDL_Flip(real_screen);
}

static void delay()
{
	SDL_framerateDelay(&fps);
}

int main(int argc, char **argv)
{
	if(argc == 1) {
		fprintf(stderr, "Usage: %s FILE\n", argv[0]);
		exit(1);
	}

	errno = 0;
	size_t size;
	int32_t *mem = read_file(argv[1], &size);
	if(!mem) {
		if(errno)
			perror(argv[0]);
		else
			fprintf(stderr, "%s: The file was invalid.\n", argv[0]);
		exit(1);
	}

	init_vm(mem, size);

	while(1) {
		run_vm();
		render_screen();
		process_events();
		delay();
	}
}
