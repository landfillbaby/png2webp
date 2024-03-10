// anti-copyright Lucy Phipps 2022
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

static inline void *u2o(const uint16_t x, void *y) {
	memcpy(y, &x, 2);
	return y;
}
static inline void *u4o(const uint32_t x, void *y) {
	memcpy(y, &x, 4);
	return y;
}
static inline void *u8o(const uint64_t x, void *y) {
	memcpy(y, &x, 8);
	return y;
}

static inline uint16_t l2(const uint8_t *x) {
	uint16_t r = x[1];
	r <<= 8;
	return r + *x;
}
static inline uint32_t l4(const uint8_t *x) {
	uint32_t r = l2(x + 2);
	r <<= 16;
	return r + l2(x);
}
static inline uint64_t l8(const uint8_t *x) {
	uint64_t r = l4(x + 4);
	r <<= 32;
	return r + l4(x);
}

static inline uint8_t *l2o(const uint16_t x, uint8_t *y) {
	y[1] = x >> 8;
	*y = (uint8_t)x;
	return y;
}
static inline uint8_t *l4o(const uint32_t x, uint8_t *y) {
	l2o(x >> 16, y + 2);
	return l2o((uint16_t)x, y);
}
static inline uint8_t *l8o(const uint64_t x, uint8_t *y) {
	l4o(x >> 32, y + 4);
	return l4o((uint32_t)x, y);
}

static inline uint16_t b2(const uint8_t *x) {
	uint16_t r = *x;
	r <<= 8;
	return r + x[1];
}
static inline uint32_t b4(const uint8_t *x) {
	uint32_t r = b2(x);
	r <<= 16;
	return r + b2(x + 2);
}
static inline uint64_t b8(const uint8_t *x) {
	uint64_t r = b4(x);
	r <<= 32;
	return r + b4(x + 4);
}

static inline uint8_t *b2o(const uint16_t x, uint8_t *y) {
	y[1] = (uint8_t)x;
	*y = x >> 8;
	return y;
}
static inline uint8_t *b4o(const uint32_t x, uint8_t *y) {
	b2o((uint16_t)x, y + 2);
	return b2o(x >> 16, y);
}
static inline uint8_t *b8o(const uint64_t x, uint8_t *y) {
	b4o((uint32_t)x, y + 4);
	return b4o(x >> 32, y);
}
#endif
