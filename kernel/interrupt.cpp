#include "interrupt.hpp"

std::array<InterruptDescriptor, 256> idt;

void SetIDTEntry(InterruptDescriptor &descriptor,
                 InterruptDescriptorAttribute attr, uint64_t offset,
                 uint16_t segment_selector) {
    descriptor.attr = attr;
    descriptor.offset_low = offset & 0xffffu;
    descriptor.offset_middle = (offset >> 16) & 0xffffu;
    descriptor.offset_high = offset >> 32;
    descriptor.segment_selector = segment_selector;
}

void NotifyEndOfInterrupt() {
    volatile auto end_of_interrupt = reinterpret_cast<uint32_t *>(0xfee000b0);
    *end_of_interrupt = 0;
}