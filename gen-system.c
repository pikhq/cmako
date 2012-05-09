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

int main(int argc, char **argv)
{
	printf("#include <stdint.h>\n"
	       "#include <SDL.h>\n"
	       "#include \"mako-vm.h\"\n"
	       "#ifndef NAME\n"
	       "#define NAME \"\"\n"
	       "#endif\n"
	       "char *argv0;\n"
	       "int32_t mem[] = {");
	while(!feof(stdin)) {
		uint8_t buf[4];
		int n = fread(buf, sizeof *buf, 4, stdin);
		if(ferror(stdin)) goto onerr;
		if(n == 0) break;
		if(n != 4) {
			fprintf(stderr, "%s: The file was invalid.\n", argv[0]);
			exit(1);
		}
		printf("0x%04X,", (int32_t)buf[0] << 24 | (int32_t)buf[1] << 16 | (int32_t)buf[2] << 8 | (int32_t)buf[3]);
	}
	printf("0};\n"
	       "int main(int argc, char **argv)\n"
	       "{\n"
	       "\targv0 = argv[0];\n"
	       "\trun_vm(mem, NAME);\n"
	       "}\n");
	exit(0);

onerr:
	perror(argv[0]);
	exit(1);
}
