#pragma once
#define WDTO_15MS 0
#include <stdlib.h>
static inline void wdt_enable(int) { abort(); }   // the OTA abort path: never reached in tests
