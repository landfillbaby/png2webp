// vi: sw=2 tw=80
#ifndef LE_H
#define LE_H
#include <stdint.h>
// little to host
#define LH16(x) ((uint16_t)(*(x) | (uint16_t)(x)[1] << 8))
#define LH(x) ((uint32_t)(LH16(x) | (uint32_t)LH16(&(x)[2]) << 16))
// host to little
#define HL(x) ((const uint8_t[4]){ \
    (x) & 255u, ((x) >> 8) & 255u, ((x) >> 16) & 255u, (x) >> 24})
#endif
