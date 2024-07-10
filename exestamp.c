#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809
#endif
#include "pun.h"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#define S _fseeki64
#elif LONG_MAX > 0xfffffffe
#define S fseek
#else
struct exestamp_static_assert {
	int off_t_too_small: (off_t)0xffffffff < 0xffffffff ? -1 : 1;
};
#define S fseeko
#endif
static int help(void) {
	fputs("Usage: exestamp EXE [STAMP]\n\
EXE:   Windows PE32(+) file\n\
STAMP: new Unix timestamp,\n\
       decimal, octal (leading 0), or hexadecimal (leading 0x)\n",
		stderr);
	return -1;
}
int main(int argc, char **argv) {
	uint32_t b, t; // uninitialized warnings are false :)
	if(argc == 3) {
		if(!*argv[2] || isspace(*argv[2])) return help();
		char *n;
		t = (uint32_t)strtoull(argv[2], &n, 0);
		if(*n || errno) return help();
	} else if(argc != 2) return help();
	FILE *const f = fopen(argv[1], argc == 3 ? "rb+" : "rb");
	if(!f) {
		perror("ERROR opening file");
		return 1;
	}
#define R(x) !fread(&b, x, 1, f)
	if(R(2) || u2(&b) != u2("\x4d\x5a") || S(f, 60, SEEK_SET) || R(4)
		|| S(f, lh(b), SEEK_SET) || R(4) || b != hl(17744u)
		|| S(f, 4, SEEK_CUR) || R(4)) {
		fputs("ERROR: Invalid Windows PE32(+) file\n", stderr);
		fclose(f);
		return 1;
	}
	if(argc == 3) {
		printf("old: %" PRIu32 "\nnew: %" PRIu32 "\n", lh(b), t);
		t = hl(t);
		if(S(f, -4, SEEK_CUR) || !fwrite(&t, 4, 1, f)) {
			perror("ERROR writing new timestamp");
			fclose(f);
			return 1;
		}
		if(fclose(f)) {
			perror("ERROR writing new timestamp");
			return 1;
		}
	} else {
		fclose(f);
		printf("%" PRIu32 "\n", lh(b));
	}
}
