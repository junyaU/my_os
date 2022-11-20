#include "segment.hpp"

#include "asmfunc.h"

namespace {
std::array<SegmentDescriptor, 3> gdt;
}

void SetCodeSegment(SegmentDescriptor& descriptor, DescriptorType type,
                    unsigned int privilege_level, uint32_t base,
                    uint32_t limit) {
    descriptor.data = 0;
    descriptor.bits.base_low = base & 0xffffu;
    descriptor.bits.base_middle = (base >> 16) & 0xffu;
    descriptor.bits.base_high = (base >> 24) & 0xffu;
    descriptor.bits.limit_low = limit & 0xffffu;
    descriptor.bits.type = type;
    descriptor.bits.system_segment = 1;
    descriptor.bits.descriptor_privilege_level = privilege_level;
    descriptor.bits.present = 1;
    // osが使える領域
    descriptor.bits.available = 0;
    descriptor.bits.long_mode = 1;  // 64bitモード
    descriptor.bits.default_operation_size = 0;
    descriptor.bits.granularity = 1;
}

void SetDataSegment(SegmentDescriptor& descriptor, DescriptorType type,
                    unsigned int privilege_level, uint32_t base,
                    uint32_t limit) {
    SetCodeSegment(descriptor, type, privilege_level, base, limit);
    descriptor.bits.long_mode = 0;
    descriptor.bits.default_operation_size = 1;
}

void SetupSegments() {
    gdt[0].data = 0;
    SetCodeSegment(gdt[1], DescriptorType::kExecuteRead, 0, 0, 0xfffff);
    SetDataSegment(gdt[2], DescriptorType::kReadWrite, 0, 0, 0xfffff);
    LoadGDT(sizeof(gdt) - 1, reinterpret_cast<uintptr_t>(&gdt[0]));
}