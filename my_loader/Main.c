#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Protocol/DiskIo2.h>
#include  <Protocol/BlockIo.h>
#include  <Guid/FileInfo.h>

struct MemoryMap {
    UINTN buffer_size; // メモリマップのサイズ
    VOID *buffer; //メモリマップの先頭アドレス
    UINTN map_size;  // ?
    UINTN map_key; //mapの状態を一意に表すキー
    UINTN descriptor_size; // メモリマップの一要素のサイズ？
    UINT32 deccriptor_version; // バージョン
};

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
    switch (type) {
        case EfiReservedMemoryType: return L"EfiReservedMemoryType";
        case EfiLoaderCode: return L"EfiLoaderCode";
        case EfiLoaderData: return L"EfiLoaderData";
        case EfiBootServicesCode: return L"EfiBootServicesCode";
        case EfiBootServicesData: return L"EfiBootServicesData";
        case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
        case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
        case EfiConventionalMemory: return L"EfiConventionalMemory";
        case EfiUnusableMemory: return L"EfiUnusableMemory";
        case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
        case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
        case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
        case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
        case EfiPalCode: return L"EfiPalCode";
        case EfiPersistentMemory: return L"EfiPersistentMemory";
        case EfiMaxMemoryType: return L"EfiMaxMemoryType";
        default: return L"InvalidMemoryType";
    }
}

EFI_STATUS GetMemoryMap(struct MemoryMap *map) {
    if (map->buffer == NULL) {
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size = map->buffer_size;
    return gBS->GetMemoryMap(
        &map->map_size,
        (EFI_MEMORY_DESCRIPTOR*)map->buffer,
        &map->map_key,
        &map->descriptor_size,
        &map->deccriptor_version
    );
}

EFI_STATUS SaveMemoryMap(struct MemoryMap *map, EFI_FILE_PROTOCOL *file){
    CHAR8 buf[256];
    UINTN len;

    CHAR8 *header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    file->Write(file, &len, header);

    Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer, map->map_size);
    
    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for (
        iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
        iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
        iter += map->descriptor_size, i++
    ) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR*)iter;
        len = AsciiSPrint(
            buf,
            sizeof(buf),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            i,
            desc->Type,
            GetMemoryTypeUnicode(desc->Type),
            desc->PhysicalStart,
            desc->NumberOfPages,
            desc->Attribute & 0xffffflu
        );

        file->Write(file, &len, buf);
    }

    return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL **root) {
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

    gBS->OpenProtocol(
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );

    gBS->OpenProtocol(
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&fs,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );

    fs->OpenVolume(fs, root);

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    Print(L"hello uruunari\n");

    CHAR8 memory_map_buffer[4096 * 4];
    struct MemoryMap memory_map = {sizeof(memory_map_buffer), memory_map_buffer, 0, 0, 0, 0};
    GetMemoryMap(&memory_map);

    EFI_FILE_PROTOCOL *root_dir;
    OpenRootDir(image_handle, &root_dir);

    EFI_FILE_PROTOCOL *memory_map_file;
    root_dir->Open(
        root_dir,
        &memory_map_file,
        L"\\memmap",
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        0
    );

    SaveMemoryMap(&memory_map, memory_map_file);
    memory_map_file->Close(memory_map_file);

    // カーネルファイルを開く
    EFI_FILE_PROTOCOL *kernel_file;
    root_dir->Open(
        root_dir,
        &kernel_file,
        L"\\kernel.elf",
        EFI_FILE_MODE_READ,
        0
    );

    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];

    kernel_file->GetInfo(
        kernel_file,
        &gEfiFileInfoGuid,
        &file_info_size,
        file_info_buffer
    );

    EFI_FILE_INFO *file_info = (EFI_FILE_INFO*)file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

    // 指定したアドレスからファイルサイズ分のアドレスを確保する (メモリ領域は EfiLoaderDataとする)
    EFI_PHYSICAL_ADDRESS kernel_base_address = 0x100000;
    gBS->AllocatePages(
        AllocateAddress,
        EfiLoaderData,
        (kernel_file_size + 0xfff) / 0x1000,
        &kernel_base_address
    );

    // 指定したアドレスにカーネルファイルを読み込む
    kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_address);
    Print(L"Kernel: 0x%0lx ()%lu bytes\n", kernel_base_address, kernel_file_size);

    if (EFI_ERROR(GetMemoryMap(&memory_map))) {
        Print(L"fail to get memory map\n");
        while(1);
    }

    if (EFI_ERROR(gBS->ExitBootServices(image_handle, memory_map.map_key))) {
        Print(L"could not exit boot service\n");
        while(1);
    }

    UINT64 kernel_entry_address = *(UINT64*)(kernel_base_address + 24);

    typedef void EntryPointType(void);
    ((EntryPointType*)kernel_entry_address)();

    Print(L"All done\n");
    while (1);

    return EFI_SUCCESS;
}