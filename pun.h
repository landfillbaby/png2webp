#ifndef PUN_H
#define PUN_H
#include <limits.h>
#include <stdint.h>
#include <string.h>
#if CHAR_BIT != 8
#error "char isn't 8-bit"
#endif
#define U uint8_t
#define U2 uint16_t
#define U4 uint32_t
static inline U2 u2(const void *x) {
	U2 r;
	memcpy(&r, x, 2);
	return r;
}
static inline U4 u4(const void *x) {
	U4 r;
	memcpy(&r, x, 4);
	return r;
}
static inline U4 lh(U4 x) { // little to host
	U y[4];
	memcpy(y, &x, 4);
	return (U4)*y | ((U4)y[1] << 8) | ((U4)y[2] << 16) | ((U4)y[3] << 24);
}
static inline U4 hl(U4 x) { // host to little
	return u4((U[]){(U)x, (U)(x >> 8), (U)(x >> 16), (U)(x >> 24)});
}
#endif
