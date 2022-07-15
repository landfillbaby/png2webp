// anti-copyright Lucy Phipps 2022
// vi: sw=2 tw=80
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if CHAR_BIT != 8
#error "char isn't 8-bit"
#endif
int main(int argc, char **argv) {
  if(argc != 3 || *argv[2] < '0' || *argv[2] > '9') {
  h:
    fputs("Usage: exestamp EXE STAMP\nEXE: Windows PE32(+) file\nSTAMP: \
Decimal, octal (leading 0), or hexadecimal (leading 0x) Unix timestamp\n",
      stderr);
    return -1;
  }
  char *n;
  uint32_t t = (uint32_t)strtoll(argv[2], &n, 0);
  if(*n || errno) goto h;
  FILE *f = fopen(argv[1], "rb+");
  if(!f) {
    fputs("Couldn't open file\n", stderr);
    return 1;
  }
  uint8_t b[4];
  if(!fread(b, 2, 1, f)) {
  e:
    fputs("Invalid Windows PE32(+) file\n", stderr);
    fclose(f);
    return 1;
  }
#define B (uint32_t)((b[3] << 24) | (b[2] << 16) | (b[1] << 8) | *b)
  if(memcmp(b, (char[2]){"MZ"}, 2) || fseek(f, 60, SEEK_SET) ||
    !fread(b, 4, 1, f) || fseek(f, B, SEEK_SET) || !fread(b, 4, 1, f) ||
    memcmp(b, "PE\0", 4) || fseek(f, 4, SEEK_CUR))
    goto e;
#ifndef NO_PRINT_ORIG
  if(!fread(b, 4, 1, f)) goto e;
  fprintf(stderr, "Original timestamp: %" PRIu32 "\n", B);
  if(fseek(f, -4, SEEK_CUR)) goto e;
#endif
  if(!fwrite(
       (char[]){(char)t, (char)(t >> 8), (char)(t >> 16), (char)(t >> 24)}, 4,
       1, f)) {
    fputs("Couldn't write new timestamp\n", stderr);
    fclose(f);
    return 1;
  }
  if(fclose(f)) {
    fputs("Error while closing file\n", stderr);
    return 1;
  }
}
