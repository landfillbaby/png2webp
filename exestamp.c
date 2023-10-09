// anti-copyright Lucy Phipps 2022
// vi: sw=2 tw=80
#define _FILE_OFFSET_BITS 64
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
struct static_assert_old {
  int off_t_too_small: (off_t)0xffffffff < 0xffffffffll ? -1 : 1;
};
#define S fseeko
#endif
int main(int argc, char **argv) {
  char *n;
  uint32_t t;
  if(argc < 2 || argc > 3 || isspace(*argv[2])
      || ((void)(t = (uint32_t)strtoll(argv[2], &n, 0)), *n) || errno) {
    fputs("Usage: exestamp EXE [STAMP]\n\
EXE:   Windows PE32(+) file\n\
STAMP: new Unix timestamp,\n\
       decimal, octal (leading 0), or hexadecimal (leading 0x)\n",
	stderr);
    return -1;
  }
  FILE *f = fopen(argv[1], argc < 3 ? "rb" : "rb+");
  if(!f) {
    perror("ERROR opening file");
    return 1;
  }
  uint8_t b[4];
#define B (uint32_t)(*b | (b[1] << 8) | (b[2] << 16) | (b[3] << 24))
#define R(x) !fread(b, x, 1, f)
  if(R(2) || u2(b) != u2("MZ") || S(f, 60, SEEK_SET) || R(4)
      || S(f, B, SEEK_SET) || R(4) || u4(b) != u4("PE\0") || S(f, 4, SEEK_CUR)
      || R(4)) {
    fputs("ERROR: Invalid Windows PE32(+) file\n", stderr);
    fclose(f);
    return 1;
  }
  if(argc < 3) printf("%" PRIu32 "\n", B);
  else {
    printf("old: %" PRIu32 "\nnew: %" PRIu32 "\n", B, t);
#define T(x) ((t >> x) & 255)
    if(S(f, -4, SEEK_CUR)
	|| !fwrite((uint8_t[]){T(0), T(8), T(16), T(24)}, 4, 1, f)) {
      perror("ERROR writing new timestamp");
      fclose(f);
      return 1;
    }
  }
  if(fclose(f)) {
    perror("ERROR closing file");
    return 1;
  }
}
