#pragma once

#include <cstddef>
#include <cstdint>

const size_t kPageDirectoryCount = 64;

// CR3レジスタにページテーブル設定　以降はこのページテーブルを参照するようになる
void SetupIdentityPageTable();

void InitializePaging();

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