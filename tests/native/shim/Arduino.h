// Host shim: just enough Arduino for PinNodeCtrl.cpp to build and run natively (tests only).
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t byte;
typedef bool boolean;
extern unsigned long fake_millis;
static inline unsigned long millis() { return fake_millis; }
#define PROGMEM
#define pgm_read_dword(p) (*(const uint32_t *)(p))
extern const uint8_t *fake_flash;
extern uint32_t fake_flash_len;
#define pgm_get_far_address(x) (fake_flash_len)
#define pgm_read_byte_far(a) (fake_flash[(a)])

#define F(x) (x)
struct Print {
  virtual size_t write(uint8_t) = 0;
  size_t print(const char *s) { size_t n = 0; while (*s) n += write((uint8_t)*s++); return n; }
  size_t print(unsigned long v) { char b[16]; snprintf(b, sizeof b, "%lu", v); return print(b); }
  size_t print(unsigned int v) { return print((unsigned long)v); }
  size_t println(const char *s) { size_t n = print(s); return n + print("\n"); }
  size_t println(unsigned long v) { size_t n = print(v); return n + print("\n"); }
  size_t println(unsigned int v) { return println((unsigned long)v); }
  virtual ~Print() {}
};
extern uint8_t SREG;
static inline void cli() {}
