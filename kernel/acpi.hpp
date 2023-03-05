#pragma once

#include <cstdint>

// advanced configuration and power interface
namespace acpi {
//  root system description pointer
struct RSDP {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    // RSDPのバージョン番号
    uint8_t revision;
    uint32_t rsdt_address;
    // RSDPのバイト数
    uint32_t length;
    uint64_t xsdt_address;
    // 拡張領域のチェックサム
    uint8_t extended_checksum;
    // 予約領域
    char reserved[3];

    bool IsValid() const;
} __attribute__((packed));

struct DescriptionHeader {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[16];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;

    bool IsValid(const char* expected_signature) const;
} __attribute__((packed));

// extended system description table
struct XSDT {
    DescriptionHeader header;
    const DescriptionHeader& operator[](size_t i) const;
    size_t Count() const;
} __attribute__((packed));

struct FADT {
    DescriptionHeader header;
    char reserved1[76 - sizeof(header)];
    uint32_t pm_tmr_blk;
    char reserved2[112 - 80];
    uint32_t flags;
    char reserved3[276 - 116];
} __attribute__((packed));

extern const FADT* fadt;

void Initialize(const RSDP& rsdp);
}  // namespace acpi