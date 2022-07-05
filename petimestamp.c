// anti-copyright Lucy Phipps 2022
// vi: sw=2 tw=80
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
static uint32_t le32(uint8_t x[4]) {
  return (uint32_t)((x[3] << 24) | (x[2] << 16) | (x[1] << 8) | x[0]);
}
int main(int argc, char **argv) {
  static_assert(CHAR_BIT == 8, "char isn't 8-bit");
  if(argc != 3) {
    fputs("Usage: petimestamp <file> <timestamp>\n", stderr);
    return -1;
  }
  char *end;
  uintmax_t t = strtoumax(argv[2], &end, 0);
  if(t > 0xFFFFFFFF || *end) {
    fputs("Invalid timestamp!\n"
	  "Should be decimal, octal (leading 0), or hexadecimal (leading 0x),\n"
	  "as seconds since the epoch, in range 0 to 0xFFFFFFFF\n",
      stderr);
    return 1;
  }
  FILE *f = fopen(argv[1], "r+b");
  if(!f) {
    fputs("Couldn't open file\n", stderr);
    return 1;
  }
  char dos[2];
  if(!fread(dos, 2, 1, f)) {
  bad:
    fputs("Invalid Windows PE(64) file\n", stderr);
    fclose(f);
    return 1;
  }
  if(*dos != 'M' || dos[1] != 'Z') goto bad;
  if(fseek(f, 60, SEEK_SET)) goto bad;
  uint8_t buf[4];
  if(!fread(buf, 4, 1, f)) goto bad;
  uint32_t offset = le32(buf);
  if(fseek(f, offset, SEEK_SET)) goto bad;
  if(!fread(buf, 4, 1, f)) goto bad;
  if(*buf != 'P' || buf[1] != 'E' || buf[2] || buf[3]) goto bad;
  if(fseek(f, offset + 8, SEEK_SET)) goto bad;
#ifndef NO_PRINT_ORIG
  if(!fread(buf, 4, 1, f)) goto bad;
  uint32_t origts = le32(buf);
  fprintf(stderr, "Original timestamp: %" PRIu32 "\n", origts);
  if(fseek(f, offset + 8, SEEK_SET)) goto bad;
#endif
  if(!fwrite(
       (char[4]){(char)t, (char)(t >> 8), (char)(t >> 16), (char)(t >> 24)}, 4,
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
