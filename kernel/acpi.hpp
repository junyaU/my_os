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

void Initialize(const RSDP& rsdp);
}  // namespace acpi