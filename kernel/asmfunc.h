#pragma once

#include <stdint.h>

extern "C" {
void LoadGDT(uint16_t limit, uint64_t offset);
void SetDSAll(uint16_t value);
void SetCSSS(uint16_t cs, uint16_t ss);
void SetCR3(uint64_t value);
}
