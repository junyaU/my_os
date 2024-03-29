#include "paging.hpp"

#include <array>

#include "asmfunc.h"
#include "error.hpp"
#include "memory_manager.hpp"
#include "task.hpp"

namespace {
const uint64_t kPageSize4K = 4096;
const uint64_t kPageSize2M = 512 * kPageSize4K;
const uint64_t kPageSize1G = 512 * kPageSize2M;

alignas(kPageSize4K) std::array<uint64_t, 512> pml4_table;
alignas(kPageSize4K) std::array<uint64_t, 512> pdp_table;
alignas(kPageSize4K)
    std::array<std::array<uint64_t, 512>, kPageDirectoryCount> page_directory;
}  // namespace

// アイデンティティマッピングの設定を行う
void SetupIdentityPageTable() {
    pml4_table[0] = reinterpret_cast<uint64_t>(&pdp_table[0]) | 0x003;
    for (int i_pdpt = 0; i_pdpt < page_directory.size(); ++i_pdpt) {
        pdp_table[i_pdpt] =
            reinterpret_cast<uint64_t>(&page_directory[i_pdpt]) | 0x003;
        for (int i_pd = 0; i_pd < 512; ++i_pd) {
            page_directory[i_pdpt][i_pd] =
                i_pdpt * kPageSize1G + i_pd * kPageSize2M | 0x083;
        }
    }
    SetCR3(reinterpret_cast<uint64_t>(&pml4_table[0]));
    SetCR0(GetCR0() & 0xfffeffff);
}

void InitializePaging() { SetupIdentityPageTable(); }

void ResetCR3() { SetCR3(reinterpret_cast<uint64_t>(&pml4_table[0])); }

namespace {
WithError<PageMapEntry*> SetNewPageMapIfNotPresent(PageMapEntry& entry) {
    if (entry.bits.present) {
        return {entry.Pointer(), MAKE_ERROR(Error::kSuccess)};
    }

    auto [child_map, err] = NewPageMap();
    if (err) {
        return {nullptr, err};
    }

    entry.SetPointer(child_map);
    entry.bits.present = 1;

    return {child_map, MAKE_ERROR(Error::kSuccess)};
}

WithError<size_t> SetupPageMap(PageMapEntry* page_map, int page_map_level,
                               LinearAddress4Level addr, size_t num_4kpages,
                               bool writable) {
    while (num_4kpages > 0) {
        const auto entry_index = addr.Part(page_map_level);

        auto [child_map, err] =
            SetNewPageMapIfNotPresent(page_map[entry_index]);
        if (err) {
            return {num_4kpages, err};
        }

        page_map[entry_index].bits.user = 1;

        if (page_map_level == 1) {
            page_map[entry_index].bits.writable = writable;
            --num_4kpages;
        } else {
            page_map[entry_index].bits.writable = true;
            auto [num_remain_pages, err] = SetupPageMap(
                child_map, page_map_level - 1, addr, num_4kpages, writable);
            if (err) {
                return {num_4kpages, err};
            }
            num_4kpages = num_remain_pages;
        }

        if (entry_index == 511) {
            break;
        }

        addr.SetPart(page_map_level, entry_index + 1);
        for (int level = page_map_level - 1; level >= 1; --level) {
            addr.SetPart(level, 0);
        }
    }

    return {num_4kpages, MAKE_ERROR(Error::kSuccess)};
}

Error CleanPageMap(PageMapEntry* page_map, int page_map_level,
                   LinearAddress4Level addr) {
    for (int i = addr.Part(page_map_level); i < 512; i++) {
        auto entry = page_map[i];
        if (!entry.bits.present) {
            continue;
        }

        if (page_map_level > 1) {
            if (auto err =
                    CleanPageMap(entry.Pointer(), page_map_level - 1, addr)) {
                return err;
            }
        }

        if (entry.bits.writable) {
            const auto entry_addr =
                reinterpret_cast<uintptr_t>(entry.Pointer());
            const FrameID map_frame{entry_addr / kBytesPerFrame};
            if (auto err = memory_manager->Free(map_frame, 1)) {
                return err;
            }
        }
        page_map[i].data = 0;
    }

    return MAKE_ERROR(Error::kSuccess);
}

const FileMapping* FindFileMapping(const std::vector<FileMapping>& fmaps,
                                   uint64_t casual_vaddr) {
    for (const FileMapping& m : fmaps) {
        if (m.vaddr_begin <= casual_vaddr && casual_vaddr < m.vaddr_end) {
            return &m;
        }
    }

    return nullptr;
}

Error PreparePageCache(FileDescriptor& fd, const FileMapping& m,
                       uint64_t casual_vaddr) {
    LinearAddress4Level addr{casual_vaddr};
    addr.parts.offset = 0;
    if (auto err = SetupPageMaps(addr, 1)) {
        return err;
    }

    const long file_offset = addr.value - m.vaddr_begin;
    void* page_cache = reinterpret_cast<void*>(addr.value);
    fd.Load(page_cache, 4096, file_offset);

    return MAKE_ERROR(Error::kSuccess);
}

Error SetPageContent(PageMapEntry* table, int part, LinearAddress4Level addr,
                     PageMapEntry* content) {
    if (part == 1) {
        const auto i = addr.Part(part);
        table[i].SetPointer(content);
        table[i].bits.writable = 1;
        InvalidateTLB(addr.value);
        return MAKE_ERROR(Error::kSuccess);
    }

    const auto i = addr.Part(part);
    return SetPageContent(table[i].Pointer(), part - 1, addr, content);
}

Error CopyOnePage(uint64_t casual_addr) {
    auto [page_map, err] = NewPageMap();
    if (err) {
        return err;
    }

    const auto aligned_addr = casual_addr & 0xffff'ffff'ffff'f000;
    memcpy(page_map, reinterpret_cast<void*>(aligned_addr), 4096);
    return SetPageContent(reinterpret_cast<PageMapEntry*>(GetCR3()), 4,
                          LinearAddress4Level{casual_addr}, page_map);
}

}  // namespace

WithError<PageMapEntry*> NewPageMap() {
    auto frame = memory_manager->Allocate(1);
    if (frame.error) {
        return {nullptr, frame.error};
    }

    auto e = reinterpret_cast<PageMapEntry*>(frame.value.Frame());
    memset(e, 0, sizeof(uint64_t) * 512);
    return {e, MAKE_ERROR(Error::kSuccess)};
}

Error SetupPageMaps(LinearAddress4Level addr, size_t num_4kpages,
                    bool writable) {
    auto pml4_table = reinterpret_cast<PageMapEntry*>(GetCR3());
    return SetupPageMap(pml4_table, 4, addr, num_4kpages, writable).error;
}

Error CleanPageMaps(LinearAddress4Level addr) {
    auto pml4_table = reinterpret_cast<PageMapEntry*>(GetCR3());
    return CleanPageMap(pml4_table, 4, addr);
}

Error CopyPageMaps(PageMapEntry* dst, PageMapEntry* src, int part, int start) {
    // page
    bool is_page_level = part == 1;
    if (is_page_level) {
        for (int i = start; i < 512; i++) {
            if (!src[i].bits.present) {
                continue;
            }

            dst[i] = src[i];
            dst[i].bits.writable = 0;
        }

        return MAKE_ERROR(Error::kSuccess);
    }

    for (int i = start; i < 512; i++) {
        if (!src[i].bits.present) {
            continue;
        }

        auto [table, err] = NewPageMap();
        if (err) {
            return err;
        }

        dst[i] = src[i];
        dst[i].SetPointer(table);
        if (auto err = CopyPageMaps(table, src[i].Pointer(), part - 1, 0)) {
            return err;
        }
    }

    return MAKE_ERROR(Error::kSuccess);
}

Error HandlePageFault(uint64_t error_code, uint64_t casual_addr) {
    auto& task = task_manager->CurrentTask();
    const bool present = (error_code >> 0) & 1;
    const bool rw = (error_code >> 1) & 1;
    const bool user = (error_code >> 2) & 1;

    if (present && rw && user) {
        return CopyOnePage(casual_addr);
    } else if (present) {
        return MAKE_ERROR(Error::kAlreadyAllocated);
    }

    if (task.DPagingBegin() <= casual_addr && casual_addr < task.DPagingEnd()) {
        return SetupPageMaps(LinearAddress4Level{casual_addr}, 1);
    }

    if (auto m = FindFileMapping(task.FileMaps(), casual_addr)) {
        return PreparePageCache(*task.Files()[m->fd], *m, casual_addr);
    }

    return SetupPageMaps(LinearAddress4Level{casual_addr}, 1);
}