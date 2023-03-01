#pragma once

#include <stdint.h>

struct MemoryMap {
    unsigned long long bufffer_size;
    void* buffer;                 // base addr
    unsigned long long map_size;  // ?
    unsigned long long map_key;
    unsigned long long descriptor_size;
    uint32_t descriptor_version;
};

struct MemoryDescriptor {
    uint32_t type;  // 何に使われているか;
    uintptr_t physical_start;
    uintptr_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
};

#ifdef __cplusplus
enum class MemoryType {
    kEfiReservedMemoryType,
    kEfiLoaderCode,        // UEFIアプリの実行コード
    kEfiLoaderData,        // UEFIアプリのデータ
    kEfiBootServicesCode,  // ブートサービスドライバの実行コード
    kEfiBootServicesData,  // ブートサービスドライバのでーた
    kEfiRuntimeServicesCode,
    kEfiRuntimeServicesData,
    kEfiConventionalMemory,
    kEfiUnusableMemory,
    kEfiACPIReclaimMemory,
    kEfiACPIMemoryNVS,
    kEfiMemoryMappedIO,
    kEfiMemoryMappedIOPortSpace,
    kEfiPalCode,
    kEfiPersistentMemory,
    kEfiMaxMemoryType
};

inline bool operator==(uint32_t lhs, MemoryType rhs) {
    return lhs == static_cast<uint32_t>(rhs);
}

inline bool operator==(MemoryType lhs, uint32_t rhs) { return rhs == lhs; }

inline bool IsAvailable(MemoryType type) {
    return type == MemoryType::kEfiBootServicesCode ||
           type == MemoryType::kEfiBootServicesData ||
           type == MemoryType::kEfiConventionalMemory;
}

const int kUEFIPageSize = 4096;

#endif