// vi: sw=2 tw=80
#ifndef LE_H
#define LE_H
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
// little to host
#define LH16(x) ((u16)(*(x) | (u16)(x)[1] << 8))
#define LH(x) \
  ((u32)(*(x) | (u32)(x)[1] << 8 | (u32)(x)[2] << 16 | (u32)(x)[3] << 24))
// host to little
#define HL(x) \
  ((const u8[4]){(u8)(x), (u8)((x) >> 8), (u8)((x) >> 16), (u8)((x) >> 24)})
#endif
