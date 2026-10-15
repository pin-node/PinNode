#pragma once
#include <stdint.h>
#define E2END 4095
struct FakeEEPROM {
  uint8_t mem[4096];
  uint8_t read(int a) { return mem[a]; }
  void update(int a, uint8_t v) { mem[a] = v; }
  void write(int a, uint8_t v) { mem[a] = v; }
};
extern FakeEEPROM EEPROM;
