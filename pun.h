#ifndef PUN_H
#define PUN_H
#include <limits.h>
#include <stdint.h>
#include <string.h>
#if CHAR_BIT != 8
#error "char isn't 8-bit"
#endif
static inline uint16_t u2(const void *const x) {
	uint16_t r;
	memcpy(&r, x, 2);
	return r;
}
static inline uint32_t u4(const void *const x) {
	uint32_t r;
	memcpy(&r, x, 4);
	return r;
}
static inline uint32_t lh(const uint32_t x) { // little to host
#define S(n) ((uint32_t)((const uint8_t *)&x)[n] << n * 8)
	return S(0) | S(1) | S(2) | S(3);
#undef S
}
static inline uint32_t hl(const uint32_t x) { // host to little
#define S(n) ((x >> n) & 255)
	return u4((const uint8_t[]){S(0), S(8), S(16), S(24)});
#undef S
}
#endif
