// vi: sw=2 tw=80
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#include "le.h"
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
  uint32_t t; // uninitialized warnings are false :)
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
  uint8_t b[4];
#define R(x) !fread(b, x, 1, f)
  if(R(2) || LH16(b) != LH16("MZ") || fseek(f, 60, SEEK_SET) || R(4)
#if LONG_MAX < 0xffffffff
      || LH(b) > (uint32_t)LONG_MAX
#endif
      || fseek(f, LH(b), SEEK_SET) || R(4) || LH(b) != LH("PE\0")
      || fseek(f, 4, SEEK_CUR) || R(4)) {
    fputs("ERROR: Invalid Windows PE32(+) file\n", stderr);
    fclose(f);
    return 1;
  }
#define M ("old: %" PRIu32 "\nnew: %" PRIu32 "\n")
  if(argc == 3) {
    printf(M, LH(b), t);
#define E perror("ERROR writing new timestamp")
    if(fseek(f, -4, SEEK_CUR) || !fwrite(HL(t), 4, 1, f)) {
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
    printf(/* "%" PRIu32 "\n" */ M + sizeof(M) - sizeof(PRIu32) - 2u, LH(b));
  }
}
