// anti-copyright Lucy Phipps 2022
// vi: sw=2 tw=80
#define _FILE_OFFSET_BITS 64
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if CHAR_BIT != 8
#error "char isn't 8-bit"
#endif
#ifdef _WIN32
#define F _fseeki64
#elif LONG_MAX > 0xfffffffe
#define F fseek
#else
struct c99_static_assert {
  int off_t_must_be_64bit: sizeof(off_t) > 4;
};
#define F fseeko
#endif
int main(int argc, char **argv) {
  char *n;
  uint32_t t;
  if(argc != 3 || isspace(*argv[2]) ||
      ((void)(t = (uint32_t)strtoll(argv[2], &n, 0)), *n) || errno) {
    fputs("Usage: exestamp EXE STAMP\nEXE: Windows PE32(+) file\nSTAMP: \
Decimal, octal (leading 0), or hexadecimal (leading 0x) Unix timestamp\n",
	stderr);
    return -1;
  }
  FILE *f = fopen(argv[1], "rb+");
  if(!f) {
    perror("ERROR opening file");
    return 1;
  }
  uint8_t b[4];
#define B (uint32_t)(*b | (b[1] << 8) | (b[2] << 16) | (b[3] << 24))
#define R(x) !fread(b, x, 1, f)
  if(R(2) || memcmp(b, (char[2]){"MZ"}, 2) || F(f, 60, SEEK_SET) || R(4) ||
      F(f, B, SEEK_SET) || R(4) || memcmp(b, "PE\0", 4) || F(f, 4, SEEK_CUR) ||
      R(4)) {
    fputs("ERROR: Invalid Windows PE32(+) file\n", stderr);
    fclose(f);
    return 1;
  }
  printf("Original timestamp: %" PRIu32 "\n", B);
#define T(x) ((t >> x) & 255)
  if(F(f, -4, SEEK_CUR) ||
      !fwrite((uint8_t[]){T(0), T(8), T(16), T(24)}, 4, 1, f)) {
    perror("ERROR writing new timestamp");
    fclose(f);
    return 1;
  }
  if(fclose(f)) {
    perror("ERROR closing file");
    return 1;
  }
}
