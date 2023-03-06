#pragma once

#include <stdint.h>

extern "C" {
void LoadGDT(uint16_t limit, uint64_t offset);

void SetDSAll(uint16_t value);

void SetCSSS(uint16_t cs, uint16_t ss);

void SetCR3(uint64_t value);

void IoOut32(uint16_t addr, uint32_t data);

// 指定したIOポートからの値を受け取る
uint32_t IoIn32(uint16_t addr);

uint16_t GetCS(void);

void LoadIDT(uint16_t limit, uint64_t offset);
}
