// anti-copyright Lucy Phipps 2022
// vi: sw=2 tw=80
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#include "pun.h"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
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
static int help(void) {
  fputs("Usage: exestamp EXE [STAMP]\n\
EXE:   Windows PE32(+) file\n\
STAMP: new Unix timestamp,\n\
       decimal, octal (leading 0), or hexadecimal (leading 0x)\n",
      stderr);
  return -1;
}
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4701)
#elif defined __GNUC__ && !defined __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
int main(int argc, char **argv) {
  uint32_t t;
#ifdef _MSC_VER
#pragma warning(pop)
#elif defined __GNUC__ && !defined __clang__
#pragma GCC diagnostic pop
#endif
  bool w;
  switch(argc) {
    case 2: w = 0; break;
    case 3:
      if(isspace(*argv[2])) return help();
      char *n;
      t = (uint32_t)strtoll(argv[2], &n, 0);
      if(*n || errno) return help();
      w = 1;
      break;
    default: return help();
  }
  FILE *f = fopen(argv[1], w ? "rb+" : "rb");
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
  if(w) {
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconditional-uninitialized"
#endif
    printf("old: %" PRIu32 "\nnew: %" PRIu32 "\n", B, t);
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#define T(x) ((t >> x) & 255)
    if(S(f, -4, SEEK_CUR)
	|| !fwrite((uint8_t[]){T(0), T(8), T(16), T(24)}, 4, 1, f)) {
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
    printf("%" PRIu32 "\n", B);
  }
  return 0;
}
