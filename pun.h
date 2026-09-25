#ifndef PUN_H
#define PUN_H
#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
static inline u16 t16(const void *x) {
  u16 r;
  memcpy(&r, x, 2);
  return r;
}
static inline u32 t32(const void *x) {
  u32 r;
  memcpy(&r, x, 4);
  return r;
}
static inline u32 lh(u32 x) { // little to host
  u8 *y = (u8 *)&x;
  return (u32)*y | ((u32)y[1] << 8) | ((u32)y[2] << 16) | ((u32)y[3] << 24);
}
static inline u32 hl(u32 x) { // host to little
  return t32((u8[]){(u8)x, (u8)(x >> 8), (u8)(x >> 16), (u8)(x >> 24)});
}
#endif
