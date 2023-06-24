#include "segment.hpp"

#include "asmfunc.h"
#include "console.hpp"
#include "memory_manager.hpp"

namespace {
std::array<SegmentDescriptor, 7> gdt;
std::array<uint32_t, 26> tss;

static_assert((kTSS >> 3) + 1 < gdt.size());
}  // namespace

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

void SetSystemSegment(SegmentDescriptor& desc, DescriptorType type,
                      unsigned int privilege_level, uint32_t base,
                      uint32_t limit) {
    SetCodeSegment(desc, type, privilege_level, base, limit);
    desc.bits.system_segment = 0;
    desc.bits.long_mode = 0;
}

// GDT再構築
void SetupSegments() {
    gdt[0].data = 0;
    SetCodeSegment(gdt[1], DescriptorType::kExecuteRead, 0, 0, 0xfffff);
    SetDataSegment(gdt[2], DescriptorType::kReadWrite, 0, 0, 0xfffff);
    SetDataSegment(gdt[3], DescriptorType::kReadWrite, 3, 0, 0xfffff);
    SetCodeSegment(gdt[4], DescriptorType::kExecuteRead, 3, 0, 0xfffff);
    // CPUにgdtを新しいGDTとして登録する
    LoadGDT(sizeof(gdt) - 1, reinterpret_cast<uintptr_t>(&gdt[0]));
}

void InitializeSegmentation() {
    SetupSegments();
    SetDSAll(kKernelDS);
    SetCSSS(kKernelCS, kKernelSS);
}

void InitializeTSS() {
    const int kRSP0Frames = 8;
    auto [stack, err] = memory_manager->Allocate(kRSP0Frames);
    if (err) {
        printk("failed to allocate rsp0: %s\n", err.Name());
        exit(1);
    }

    uint64_t rsp0 =
        reinterpret_cast<uint64_t>(stack.Frame()) + kRSP0Frames * 4096;
    tss[1] = rsp0 & 0xffffffffu;
    tss[2] = rsp0 >> 32;

    uint64_t tss_addr = reinterpret_cast<uint64_t>(&tss[0]);
    SetSystemSegment(gdt[kTSS >> 3], DescriptorType::kTSSAvailable, 0, tss_addr,
                     sizeof(tss) - 1);
    gdt[(kTSS >> 3) + 1].data = tss_addr >> 32;

    LoadTR(kTSS);
}