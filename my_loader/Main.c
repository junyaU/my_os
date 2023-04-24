#include <Guid/FileInfo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Uefi.h>

#include "elf.hpp"
#include "frame_buffer_config.hpp"
#include "memory_map.hpp"

const CHAR16 *GetMemoryTypeUnicode(EFI_MEMORY_TYPE type)
{
    switch (type)
    {
    case EfiReservedMemoryType:
        return L"EfiReservedMemoryType";
    case EfiLoaderCode:
        return L"EfiLoaderCode";
    case EfiLoaderData:
        return L"EfiLoaderData";
    case EfiBootServicesCode:
        return L"EfiBootServicesCode";
    case EfiBootServicesData:
        return L"EfiBootServicesData";
    case EfiRuntimeServicesCode:
        return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData:
        return L"EfiRuntimeServicesData";
    case EfiConventionalMemory:
        return L"EfiConventionalMemory";
    case EfiUnusableMemory:
        return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory:
        return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS:
        return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO:
        return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace:
        return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode:
        return L"EfiPalCode";
    case EfiPersistentMemory:
        return L"EfiPersistentMemory";
    case EfiMaxMemoryType:
        return L"EfiMaxMemoryType";
    default:
        return L"InvalidMemoryType";
    }
}

EFI_STATUS GetMemoryMap(struct MemoryMap *map)
{
    if (map->buffer == NULL)
    {
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size = map->bufffer_size;
    return gBS->GetMemoryMap(
        &map->map_size, (EFI_MEMORY_DESCRIPTOR *)map->buffer, &map->map_key,
        &map->descriptor_size, &map->descriptor_version);
}

EFI_STATUS SaveMemoryMap(struct MemoryMap *map, EFI_FILE_PROTOCOL *file)
{
    CHAR8 buf[256];
    UINTN len;

    CHAR8 *header =
        "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    file->Write(file, &len, header);

    Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer,
          map->map_size);

    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
         iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
         iter += map->descriptor_size, i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
        len = AsciiSPrint(buf, sizeof(buf), "%u, %x, %-ls, %08lx, %lx, %lx\n",
                          i, desc->Type, GetMemoryTypeUnicode(desc->Type),
                          desc->PhysicalStart, desc->NumberOfPages,
                          desc->Attribute & 0xffffflu);

        file->Write(file, &len, buf);
    }

    return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL **root)
{
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

    gBS->OpenProtocol(image_handle, &gEfiLoadedImageProtocolGuid,
                      (VOID **)&loaded_image, image_handle, NULL,
                      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    gBS->OpenProtocol(loaded_image->DeviceHandle,
                      &gEfiSimpleFileSystemProtocolGuid, (VOID **)&fs,
                      image_handle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    fs->OpenVolume(fs, root);

    return EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle,
                   EFI_GRAPHICS_OUTPUT_PROTOCOL **gop)
{
    UINTN num_gop_handles = 0;
    EFI_HANDLE *gop_handles = NULL;
    gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL,
                            &num_gop_handles, &gop_handles);

    gBS->OpenProtocol(gop_handles[0], &gEfiGraphicsOutputProtocolGuid,
                      (VOID **)gop, image_handle, NULL,
                      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    FreePool(gop_handles);

    return EFI_SUCCESS;
}

const CHAR16 *GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt)
{
    switch (fmt)
    {
    case PixelRedGreenBlueReserved8BitPerColor:
        return L"PixelRedGreenBlueReserved8BitPerColor";
    case PixelBlueGreenRedReserved8BitPerColor:
        return L"PixelBlueGreenRedReserved8BitPerColor";
    case PixelBitMask:
        return L"PixelBitMask";
    case PixelBltOnly:
        return L"PixelBltOnly";
    case PixelFormatMax:
        return L"PixelFormatMax";
    default:
        return L"InvalidPixelFormat";
    }
}

void Halt(void)
{
    while (1)
        __asm__("hlt");
}

void CalcLoadAddressRange(Elf64_Ehdr *ehdr, UINT64 *first, UINT64 *last)
{
    Elf64_Phdr *phdr = (Elf64_Phdr *)((UINT64)ehdr + ehdr->e_phoff);
    *first = MAX_UINT64;
    *last = 0;

    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }

        *first = MIN(*first, phdr[i].p_vaddr);
        *last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz);
    }
}

void CopyLoadSegments(Elf64_Ehdr *ehdr)
{
    Elf64_Phdr *phdr = (Elf64_Phdr *)((UINT64)ehdr + ehdr->e_phoff);
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }

        UINT64 load_segment = (UINT64)ehdr + phdr[i].p_offset;
        CopyMem((VOID *)phdr[i].p_vaddr, (VOID *)load_segment,
                phdr[i].p_filesz);

        UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
        SetMem((VOID *)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
    }
}

EFI_STATUS ReadFile(EFI_FILE_PROTOCOL *file, VOID **buffer)
{
    EFI_STATUS status;
    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];

    status = file->GetInfo(file, &gEfiFileInfoGuid, &file_info_size,
                           file_info_buffer);
    if (EFI_ERROR(status))
    {
        return status;
    }

    EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buffer;
    UINTN file_size = file_info->FileSize;

    status = gBS->AllocatePool(EfiLoaderData, file_size, buffer);
    if (EFI_ERROR(status))
    {
        return status;
    }

    return file->Read(file, &file_size, *buffer);
}

EFI_STATUS OpenBlockIoProtocolForLoadedImage(EFI_HANDLE image_handle,
                                             EFI_BLOCK_IO_PROTOCOL **block_io)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;

    status = gBS->OpenProtocol(image_handle, &gEfiLoadedImageProtocolGuid,
                               (VOID **)&loaded_image, image_handle, NULL,
                               EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status))
    {
        return status;
    }

    return gBS->OpenProtocol(
        loaded_image->DeviceHandle, &gEfiBlockIoProtocolGuid, (VOID **)block_io,
        image_handle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
}

EFI_STATUS ReadBlocks(EFI_BLOCK_IO_PROTOCOL *block_io, UINT32 media_id,
                      UINTN read_bytes, VOID **buffer)
{
    EFI_STATUS status;

    status = gBS->AllocatePool(EfiLoaderData, read_bytes, buffer);
    if (EFI_ERROR(status))
    {
        return status;
    }

    return block_io->ReadBlocks(block_io, media_id, 0, read_bytes, *buffer);
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle,
                           EFI_SYSTEM_TABLE *system_table)
{
    Print(L"hello uruunari\n");

    CHAR8 memory_map_buffer[4096 * 4];
    struct MemoryMap memory_map = {
        sizeof(memory_map_buffer), memory_map_buffer, 0, 0, 0, 0};
    if (EFI_ERROR(GetMemoryMap(&memory_map)))
    {
        Print(L"failed to get memory map\n");
        Halt();
    }

    EFI_FILE_PROTOCOL *root_dir;

    if (EFI_ERROR(OpenRootDir(image_handle, &root_dir)))
    {
        Print(L"failed to open root directory\n");
        Halt();
    }

    EFI_FILE_PROTOCOL *memory_map_file;
    root_dir->Open(
        root_dir, &memory_map_file, L"\\memmap",
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

    if (EFI_ERROR(SaveMemoryMap(&memory_map, memory_map_file)))
    {
        Print(L"failed to save memory map\n");
        Halt();
    }

    if (EFI_ERROR(memory_map_file->Close(memory_map_file)))
    {
        Print(L"failed to open GOP\n");
        Halt();
    }

    // カーネルファイルを開く
    EFI_FILE_PROTOCOL *kernel_file;
    if (EFI_ERROR(root_dir->Open(root_dir, &kernel_file, L"\\kernel.elf",
                                 EFI_FILE_MODE_READ, 0)))
    {
        Print(L"failed to open file '\\kernel.elf'\n");
        Halt();
    }

    VOID *kernel_buffer;
    if (EFI_ERROR(ReadFile(kernel_file, &kernel_buffer)))
    {
        Print(L"failed to allocate pool\n");
        Halt();
    }

    Elf64_Ehdr *kernel_ehdr = (Elf64_Ehdr *)kernel_buffer;
    UINT64 kernel_first_addrress, kernel_last_address;
    CalcLoadAddressRange(kernel_ehdr, &kernel_first_addrress,
                         &kernel_last_address);
    UINTN num_pages =
        (kernel_last_address - kernel_first_addrress + 0xfff) / 0x1000;

    if (EFI_ERROR(gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages,
                                     &kernel_first_addrress)))
    {
        Print(L"failed to allocate pages\n");
        Halt();
    }

    CopyLoadSegments(kernel_ehdr);
    Print(L"Kernel: 0x%0lx - 0x%0lx\n", kernel_first_addrress,
          kernel_last_address);

    if (EFI_ERROR(gBS->FreePool(kernel_buffer)))
    {
        Print(L"failed to free pool\n");
        Halt();
    }

    VOID *volume_image;

    EFI_FILE_PROTOCOL *volume_file;

    if ((root_dir->Open(root_dir, &volume_file, L"\\fat_disk",
                        EFI_FILE_MODE_READ, 0)) == EFI_SUCCESS)
    {
        if (EFI_ERROR(ReadFile(volume_file, &volume_image)))
        {
            Print(L"failed to read file\n");
            Halt();
        }
    }
    else
    {
        EFI_BLOCK_IO_PROTOCOL *block_io;
        if (EFI_ERROR(
                OpenBlockIoProtocolForLoadedImage(image_handle, &block_io)))
        {
            Print(L"failed to open Block I/O Protocol\n");
            Halt();
        }

        EFI_BLOCK_IO_MEDIA *media = block_io->Media;
        UINTN volume_bytes = (UINTN)media->BlockSize * (media->LastBlock + 1);
        if (volume_bytes > 16 * 1024 * 1024)
        {
            volume_bytes = 16 * 1024 * 1024;
        }

        Print(L"Reading %lu bytes (Present %d, BlockSize %u, LastBlock %u)\n",
              volume_bytes, media->MediaPresent, media->BlockSize,
              media->LastBlock);

        if (EFI_ERROR(ReadBlocks(block_io, media->MediaId, volume_bytes,
                                 &volume_image)))
        {
            Print(L"failed to read blocks\n");
            Halt();
        }
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    if (EFI_ERROR(OpenGOP(image_handle, &gop)))
    {
        Print(L"failed to open GOP\n");
        Halt();
    }

    if (EFI_ERROR(GetMemoryMap(&memory_map)))
    {
        Print(L"fail to get memory map\n");
        while (1)
            ;
    }

    if (EFI_ERROR(gBS->ExitBootServices(image_handle, memory_map.map_key)))
    {
        Print(L"could not exit boot service\n");
        while (1)
            ;
    }

    struct FrameBufferConfig config = {(UINT8 *)gop->Mode->FrameBufferBase,
                                       gop->Mode->Info->PixelsPerScanLine,
                                       gop->Mode->Info->HorizontalResolution,
                                       gop->Mode->Info->VerticalResolution, 0};

    switch (gop->Mode->Info->PixelFormat)
    {
    case PixelRedGreenBlueReserved8BitPerColor:
        config.pixel_format = kPixelRGBResv8BitPerColor;
        break;
    case PixelBlueGreenRedReserved8BitPerColor:
        config.pixel_format = kPixelBGRResv8BitPerColor;
        break;
    default:
        Print(L"Unimplemented pixel format: %d\n",
              gop->Mode->Info->PixelFormat);
        Halt();
    }

    // RSDPへのポインタ
    VOID *acpi_table = NULL;
    for (UINTN i = 0; i < system_table->NumberOfTableEntries; ++i)
    {
        if (CompareGuid(&gEfiAcpiTableGuid,
                        &system_table->ConfigurationTable[i].VendorGuid))
        {
            acpi_table = system_table->ConfigurationTable[i].VendorTable;
            break;
        }
    }

    // カーネルファイルの位置
    EFI_PHYSICAL_ADDRESS entry_offset = 24;
    UINT64 kernel_entry_address =
        *(UINT64 *)(kernel_first_addrress + entry_offset);

    // 引数にメモリマップを渡してカーネルを実行
    typedef void __attribute__((sysv_abi))
    KernelEntryPoint(const struct FrameBufferConfig *, const struct MemoryMap *,
                     const VOID *, VOID *);
    ((KernelEntryPoint *)kernel_entry_address)(&config, &memory_map, acpi_table,
                                               volume_image);

    Print(L"All done\n");
    while (1)
        ;

    return EFI_SUCCESS;
}