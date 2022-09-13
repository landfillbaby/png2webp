// anti-copyright Lucy Phipps 2022
// vi: sw=2 tw=80
#define _FILE_OFFSET_BITS 64
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
#define P(x) fputs(x "\n", stderr)
int main(int argc, char **argv) {
  char *n;
  uint32_t t;
  if(argc != 3 || *argv[2] < '0' || *argv[2] > '9' ||
    ((void)(t = (uint32_t)strtoll(argv[2], &n, 0)), *n) || errno) {
    P("Usage: exestamp EXE STAMP\nEXE: Windows PE32(+) file\nSTAMP: \
Decimal, octal (leading 0), or hexadecimal (leading 0x) Unix timestamp");
    return -1;
  }
  FILE *f = fopen(argv[1], "rb+");
  if(!f) {
    P("Couldn't open file");
    return 1;
  }
  uint8_t b[4];
#define B (uint32_t)(*b | (b[1] << 8) | (b[2] << 16) | (b[3] << 24))
  if(!fread(b, 2, 1, f) || memcmp(b, (char[2]){"MZ"}, 2) ||
    F(f, 60, SEEK_SET) || !fread(b, 4, 1, f) || F(f, B, SEEK_SET) ||
    !fread(b, 4, 1, f) || memcmp(b, "PE\0", 4) || F(f, 4, SEEK_CUR) ||
    !fread(b, 4, 1, f)) {
    P("Invalid Windows PE32(+) file");
    fclose(f);
    return 1;
  }
  fprintf(stderr, "Original timestamp: %" PRIu32 "\n", B);
  if(F(f, -4, SEEK_CUR) ||
    !fwrite((uint8_t[]){t & 255, (t >> 8) & 255, (t >> 16) & 255, t >> 24}, 4,
      1, f)) {
    P("Couldn't write new timestamp");
    fclose(f);
    return 1;
  }
  if(fclose(f)) {
    P("Error while closing file");
    return 1;
  }
}
