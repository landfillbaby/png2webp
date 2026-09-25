// vi: sw=2 tw=80
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#include "pun.h"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
static int help(void) {
  fputs("Usage: exestamp EXE [STAMP]\n\
EXE:   Windows PE32(+) file\n\
STAMP: new Unix timestamp,\n\
       decimal, octal (leading 0), or hexadecimal (leading 0x)\n",
      stderr);
  return -1;
}
int main(int argc, char **argv) {
  u32 b, t; // uninitialized warnings are false :)
  if(argc == 3) {
    if(!*argv[2] || isspace(*argv[2])) return help();
    char *n;
    t = (u32)strtoull(argv[2], &n, 0);
    if(*n || errno) return help();
  } else if(argc != 2) return help();
  FILE *const f = fopen(argv[1], argc == 3 ? "rb+" : "rb");
  if(!f) {
    perror("ERROR opening file");
    return 1;
  }
#define R(x) !fread(&b, x, 1, f)
#define S(x, y) fseek(f, x, SEEK_##y)
  if(R(2) || t16(&b) != t16("\x4d\x5a") || S(60, SET) || R(4)
#if LONG_MAX < 0xffffffff
      || lh(b) > (unsigned long)LONG_MAX
#endif
      || S(lh(b), SET) || R(4) || b != hl(17744u) || S(4, CUR) || R(4)) {
    fputs("ERROR: Invalid Windows PE32(+) file\n", stderr);
    fclose(f);
    return 1;
  }
  if(argc == 3) {
    printf("old: %" PRIu32 "\nnew: %" PRIu32 "\n", lh(b), t);
    t = hl(t);
#define E perror("ERROR writing new timestamp")
    if(S(-4, CUR) || !fwrite(&t, 4, 1, f)) {
      E;
      fclose(f);
      return 1;
    }
    if(fclose(f)) {
      E;
      return 1;
    }
  } else {
    fclose(f);
    printf("%" PRIu32 "\n", lh(b));
  }
}
