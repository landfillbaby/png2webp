// anti-copyright Lucy Phipps 2022
// vi: sw=2 tw=80
#ifndef PUN_H
#define PUN_H
#include <limits.h>
#include <stdint.h>
#include <string.h>
#if CHAR_BIT != 8
#error "char isn't 8-bit"
#endif
static inline uint16_t u2(const void *x) {
  uint16_t r;
  memcpy(&r, x, 2);
  return r;
}
static inline uint32_t u4(const void *x) {
  uint32_t r;
  memcpy(&r, x, 4);
  return r;
}
static inline uint64_t u8(const void *x) {
  uint64_t r;
  memcpy(&r, x, 8);
  return r;
}
#endif
