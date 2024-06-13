// mixed-endian not supported. if you use [bl]* functions put this at the top:
// if(pun_h_check()) exit(1);
// vi: sw=2 tw=80
#ifndef PUN_H
#define PUN_H
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if CHAR_BIT != 8
#error "char should be 8-bit"
#endif
#if !__STDC_HOSTED__
#error "should be run on an OS"
#endif
#if '\a' != 7 || '\b' != 8 || '\t' != 9 || '\n' != 10 || '\v' != 11 \
    || '\f' != 12 || '\r' != 13 || ' ' != 32 || '!' != 33 || '"' != 34 \
    || '#' != 35 || '$' != 36 || '%' != 37 || '&' != 38 || '\'' != 39 \
    || '(' != 40 || ')' != 41 || '*' != 42 || '+' != 43 || ',' != 44 \
    || '-' != 45 || '.' != 46 || '/' != 47 || '0' != 48 || '1' != 49 \
    || '2' != 50 || '3' != 51 || '4' != 52 || '5' != 53 || '6' != 54 \
    || '7' != 55 || '8' != 56 || '9' != 57 || ':' != 58 || ';' != 59 \
    || '<' != 60 || '=' != 61 || '>' != 62 || '?' != 63 || '@' != 64 \
    || 'A' != 65 || 'B' != 66 || 'C' != 67 || 'D' != 68 || 'E' != 69 \
    || 'F' != 70 || 'G' != 71 || 'H' != 72 || 'I' != 73 || 'J' != 74 \
    || 'K' != 75 || 'L' != 76 || 'M' != 77 || 'N' != 78 || 'O' != 79 \
    || 'P' != 80 || 'Q' != 81 || 'R' != 82 || 'S' != 83 || 'T' != 84 \
    || 'U' != 85 || 'V' != 86 || 'W' != 87 || 'X' != 88 || 'Y' != 89 \
    || 'Z' != 90 || '[' != 91 || '\\' != 92 || ']' != 93 || '^' != 94 \
    || '_' != 95 || '`' != 96 || 'a' != 97 || 'b' != 98 || 'c' != 99 \
    || 'd' != 100 || 'e' != 101 || 'f' != 102 || 'g' != 103 || 'h' != 104 \
    || 'i' != 105 || 'j' != 106 || 'k' != 107 || 'l' != 108 || 'm' != 109 \
    || 'n' != 110 || 'o' != 111 || 'p' != 112 || 'q' != 113 || 'r' != 114 \
    || 's' != 115 || 't' != 116 || 'u' != 117 || 'v' != 118 || 'w' != 119 \
    || 'x' != 120 || 'y' != 121 || 'z' != 122 || '{' != 123 || '|' != 124 \
    || '}' != 125 || '~' != 126
#error "execution character set should be ASCII compatible"
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
static inline uint64_t u8(const void *const x) {
  uint64_t r;
  memcpy(&r, x, 8);
  return r;
}
static inline bool pun_h_check(void) {
  const uint64_t x = 0x3132333435363738u;
  if(x != u8("12345678") && x != u8("87654321")) {
    fprintf(stderr, "ERROR: 64-bit mixed-endianness (%.8s) not supported\n",
	(const char *)&x);
    return 1;
  }
  return 0;
}
static inline bool isbe(void) { return u2("\1") != 1; }
static inline uint16_t s2(const uint16_t x) {
  return (uint16_t)((x >> 8) | (x << 8));
}
static inline uint32_t s4(const uint32_t x) {
  return (uint32_t)((x >> 24) | ((x >> 8) & 0xff00u) | ((x & 0xff00u) << 8)
      | (x << 24));
}
static inline uint64_t s8(const uint64_t x) {
  return (uint64_t)((x >> 56) | ((x >> 40) & 0xff00u) | ((x >> 24) & 0xff0000u)
      | ((x >> 8) & 0xff000000u) | ((x & 0xff000000u) << 8)
      | ((x & 0xff0000u) << 24) | ((x & 0xff00u) << 40) | (x << 56));
}
static inline uint16_t b2(const uint16_t x) { return isbe() ? x : s2(x); }
static inline uint32_t b4(const uint32_t x) { return isbe() ? x : s4(x); }
static inline uint64_t b8(const uint64_t x) { return isbe() ? x : s8(x); }
static inline uint16_t l2(const uint16_t x) { return isbe() ? s2(x) : x; }
static inline uint32_t l4(const uint32_t x) { return isbe() ? s4(x) : x; }
static inline uint64_t l8(const uint64_t x) { return isbe() ? s8(x) : x; }
#endif
