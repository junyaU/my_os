#include "memory_manager.hpp"

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