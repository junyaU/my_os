#pragma once

#include <array>
#include <limits>

#include "error.hpp"
#include "memory_map.hpp"

namespace {
constexpr unsigned long long operator""_KiB(unsigned long long kib) {
    return kib * 1024;
}

constexpr unsigned long long operator""_MiB(unsigned long long mib) {
    return mib * 1024_KiB;
}

constexpr unsigned long long operator""_GiB(unsigned long long gib) {
    return gib * 1024_MiB;
}
}  // namespace

static const auto kBytesPerFrame{4_KiB};

class FrameID {
   public:
    explicit FrameID(size_t id) : id_{id} {}
    size_t ID() const { return id_; }
    void* Frame() const {
        return reinterpret_cast<void*>(id_ * kBytesPerFrame);
    }

   private:
    size_t id_;
};

static const FrameID kNullFrame{std::numeric_limits<size_t>::max()};

class BitmapMemoryManager {
   public:
    static const auto kMaxPhysicalMemoryBytes{128_GiB};
    static const auto kFrameCount{kMaxPhysicalMemoryBytes / kBytesPerFrame};
    using BitMapElementType = unsigned long;
    static const size_t kBitsPerBitMapElement{8 * sizeof(BitMapElementType)};

    BitmapMemoryManager();

    WithError<FrameID> Allocate(size_t frame_size);
    Error Free(FrameID base_frame, size_t frame_size);

    void MarkAllocated(FrameID base_frame, size_t frame_size);
    void SetMemoryRange(FrameID base_frame, FrameID end_frame);

   private:
    std::array<BitMapElementType, kFrameCount / kBitsPerBitMapElement>
        alloc_map_;
    FrameID base_frame_;
    FrameID end_frame_;

    bool GetBit(FrameID frame_id) const;
    void SetBit(FrameID frame_id, bool alllocated);
};

extern BitmapMemoryManager* memory_manager;
void InitializeMemoryManager(const MemoryMap& mamory_map);