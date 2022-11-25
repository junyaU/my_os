#include "pci.hpp"

#include "asmfunc.h"

namespace {
using namespace pci;

uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t function,
                     uint8_t reg_addr) {
    auto shl = [](uint32_t x, unsigned int bits) { return x << bits; };

    return shl(1, 31) | shl(bus, 16) | shl(device, 11) | shl(function, 8) |
           (reg_addr & 0xfcu);
}
}  // namespace