#pragma once

#include <cstddef>
#include <cstdint>

#include "error.hpp"

const size_t kPageDirectoryCount = 64;

// CR3レジスタにページテーブル設定　以降はこのページテーブルを参照するようになる
void SetupIdentityPageTable();

void InitializePaging();

void ResetCR3();

// 仮想アドレス
union LinearAddress4Level {
    uint64_t value;

    struct {
        uint64_t offset : 12;
        uint64_t page : 9;
        uint64_t dir : 9;
        uint64_t pdp : 9;
        uint64_t pml4 : 9;
        uint64_t : 16;
    } __attribute__((packed)) parts;

    int Part(int page_map_level) const {
        switch (page_map_level) {
            case 0:
                return parts.offset;
            case 1:
                return parts.page;
            case 2:
                return parts.dir;
            case 3:
                return parts.pdp;
            case 4:
                return parts.pml4;
            default:
                return 0;
        }
    };

    void SetPart(int page_map_level, int value) {
        switch (page_map_level) {
            case 0:
                parts.offset = value;
                break;
            case 1:
                parts.page = value;
                break;
            case 2:
                parts.dir = value;
                break;
            case 3:
                parts.pdp = value;
                break;
            case 4:
                parts.pml4 = value;
                break;
        }
    }
};

union PageMapEntry {
    uint64_t data;

    struct {
        uint64_t present : 1;
        uint64_t writable : 1;
        uint64_t user : 1;
        uint64_t write_through : 1;
        uint64_t cache_disable : 1;
        uint64_t accessed : 1;
        uint64_t dirty : 1;
        uint64_t huge_page : 1;
        uint64_t global : 1;
        uint64_t : 3;

        // 　下位の階層を指す物理アドレス
        uint64_t addr : 40;
        uint64_t : 12;
    } __attribute__((packed)) bits;

    PageMapEntry* Pointer() const {
        return reinterpret_cast<PageMapEntry*>(bits.addr << 12);
    }

    void SetPointer(PageMapEntry* entry) {
        bits.addr = reinterpret_cast<uint64_t>(entry) >> 12;
    }
};

WithError<PageMapEntry*> NewPageMap();
Error FreePageMap(PageMapEntry* entry);
Error SetupPageMaps(LinearAddress4Level addr, size_t num_4kpages);
Error CleanPageMaps(LinearAddress4Level addr);
Error HandlePageFault(uint64_t error_code, uint64_t causal_addr);