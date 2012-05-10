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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "mako-vm.h"

char *argv0;

int main(int argc, char **argv)
{
	if(argc == 1) {
		fprintf(stderr, "Usage: %s FILE\n", argv[0]);
		exit(1);
	}

	argv0 = argv[0];

	int pos = 0;

	FILE *f = fopen(argv[1], "rb");
	if(!f) goto onerr;
	
	int32_t *mem = calloc(1024, sizeof *mem);
	int alloc_size = 1024;
	if(!mem) goto onerr;

	while(!feof(f)) {
		if(pos == alloc_size) {
			alloc_size *= 2;
			mem = realloc(mem, alloc_size * sizeof *mem);
			if(!mem) goto onerr;
		}

		uint8_t buf[4];

		int n = fread(buf, sizeof *buf, 4, f);
		if(ferror(f)) goto onerr;
		if(n == 0) break;
		if(n != 4) {
			fprintf(stderr, "%s: The file was invalid.\n", argv[0]);
			exit(1);
		}
		mem[pos] = (int32_t)buf[0] << 24 | (int32_t)buf[1] << 16 | (int32_t)buf[2] << 8 | (int32_t)buf[3];
		pos++;
	}

	if(fclose(f) != 0) goto onerr;

	memset(mem + pos, 0, (alloc_size - pos) * sizeof *mem);

	run_vm(mem, argv[1]);

onerr:
	perror(argv[0]);
	exit(1);
}
