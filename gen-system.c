#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	printf("#include <stdint.h>\n"
	       "#include <SDL.h>\n"
	       "#include \"mako-vm.h\"\n"
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
	       "\trun_vm(mem);\n"
	       "}\n");
	exit(0);

onerr:
	perror(argv[0]);
	exit(1);
}
