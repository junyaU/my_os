#include "memory_manager.hpp"

#include <bitset>

BitmapMemoryManager::BitmapMemoryManager()
    : alloc_map_{}, base_frame_{FrameID{0}}, end_frame_{FrameID{kFrameCount}} {}

void BitmapMemoryManager::MarkAllocated(FrameID base_frame, size_t frame_size) {
    for (size_t i = 0; i < frame_size; ++i) {
        SetBit(FrameID{base_frame.ID() + i}, true);
    }
}

void BitmapMemoryManager::SetMemoryRange(FrameID base_frame,
                                         FrameID end_frame) {
    base_frame_ = base_frame;
    end_frame_ = end_frame;
}

MemoryStat BitmapMemoryManager::Stat() const {
    size_t sum = 0;
    for (int i = base_frame_.ID() / kBitsPerBitMapElement;
         i < end_frame_.ID() / kBitsPerBitMapElement; i++) {
        sum += std::bitset<kBitsPerBitMapElement>(alloc_map_[i]).count();
    }

    return {sum, end_frame_.ID() - base_frame_.ID()};
}

bool BitmapMemoryManager::GetBit(FrameID frame_id) const {
    auto line_index = frame_id.ID() / kBitsPerBitMapElement;
    auto bit_index = frame_id.ID() % kBitsPerBitMapElement;

    return (alloc_map_[line_index] &
            (static_cast<BitMapElementType>(1) << bit_index)) != 0;
}

void BitmapMemoryManager::SetBit(FrameID frame_id, bool is_allocated) {
    auto line_index = frame_id.ID() / kBitsPerBitMapElement;
    auto bit_index = frame_id.ID() % kBitsPerBitMapElement;

    if (is_allocated) {
        alloc_map_[line_index] |=
            (static_cast<BitMapElementType>(1) << bit_index);
    } else {
        alloc_map_[line_index] &=
            ~(static_cast<BitMapElementType>(1) << bit_index);
    }
}

WithError<FrameID> BitmapMemoryManager::Allocate(size_t frame_size) {
    size_t start_frame_id = base_frame_.ID();
    while (true) {
        size_t i = 0;
        for (; i < frame_size; ++i) {
            if (start_frame_id + i >= end_frame_.ID()) {
                return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
            }

            if (GetBit(FrameID{start_frame_id + i})) {
                break;
            }
        }

        if (i == frame_size) {
            MarkAllocated(FrameID{start_frame_id}, frame_size);
            return {
                FrameID{start_frame_id},
                MAKE_ERROR(Error::kSuccess),
            };
        }

        start_frame_id += i + 1;
    }
}

Error BitmapMemoryManager::Free(FrameID base_frame, size_t frame_size) {
    for (size_t i = 0; i < frame_size; ++i) {
        SetBit(FrameID{base_frame.ID() + i}, false);
    }

    return MAKE_ERROR(Error::kSuccess);
}

extern "C" caddr_t program_break, program_break_end;

namespace {
char memory_manager_buf[sizeof(BitmapMemoryManager)];

Error InitializeHeap(BitmapMemoryManager& memory_manager) {
    const int kHeapFrames = 64 * 512;
    const auto heap_start = memory_manager.Allocate(kHeapFrames);
    if (heap_start.error) {
        return heap_start.error;
    }

    program_break =
        reinterpret_cast<caddr_t>(heap_start.value.ID() * kBytesPerFrame);
    program_break_end = program_break + kHeapFrames * kBytesPerFrame;
    return MAKE_ERROR(Error::kSuccess);
}
}  // namespace

BitmapMemoryManager* memory_manager;

void InitializeMemoryManager(const MemoryMap& memory_map) {
    ::memory_manager = new (memory_manager_buf) BitmapMemoryManager;

    const auto memory_map_base_addr =
        reinterpret_cast<uintptr_t>(memory_map.buffer);
    uintptr_t available_end = 0;

    for (uintptr_t iter = memory_map_base_addr;
         iter < memory_map_base_addr + memory_map.map_size;
         iter += memory_map.descriptor_size) {
        auto descriptor = reinterpret_cast<const MemoryDescriptor*>(iter);
        if (available_end < descriptor->physical_start) {
            memory_manager->MarkAllocated(
                FrameID{available_end / kBytesPerFrame},
                (descriptor->physical_start - available_end) / kBytesPerFrame);
        }

        const auto physical_end = descriptor->physical_start +
                                  descriptor->number_of_pages * kUEFIPageSize;
        if (IsAvailable(static_cast<MemoryType>(descriptor->type))) {
            available_end = physical_end;
        } else {
            memory_manager->MarkAllocated(
                FrameID{descriptor->physical_start / kBytesPerFrame},
                descriptor->number_of_pages * kUEFIPageSize / kBytesPerFrame);
        }
    }
    memory_manager->SetMemoryRange(FrameID{1},
                                   FrameID{available_end / kBytesPerFrame});

    if (auto err = InitializeHeap(*memory_manager)) {
        exit(1);
    }
}