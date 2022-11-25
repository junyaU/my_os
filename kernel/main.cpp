#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "asmfunc.h"
#include "console.hpp"
#include "drawing.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "memory_manager.hpp"
#include "memory_map.hpp"
#include "paging.hpp"
#include "segment.hpp"

// void *operator new(size_t size, void *buf) noexcept { return buf; }

void operator delete(void *obj) noexcept {}

const PixelColor desktop_bg_color{45, 118, 237};

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;

const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ", "@@             ", "@.@            ", "@..@           ",
    "@...@          ", "@....@         ", "@.....@        ", "@......@       ",
    "@.......@      ", "@........@     ", "@.........@    ", "@..........@   ",
    "@...........@  ", "@............@ ", "@......@@@@@@@@", "@......@       ",
    "@....@@.@      ", "@...@ @.@      ", "@..@   @.@     ", "@.@    @.@     ",
    "@@      @.@    ", "@       @.@    ", "         @.@   ", "         @@@   ",
};

char screen_drawer_buffer[sizeof(RGB8BitScreenDrawer)];
ScreenDrawer *screen_drawer;

char console_buf[sizeof(Console)];
Console *console;

char memory_manager_buffer[sizeof(BitmapMemoryManager)];
BitmapMemoryManager *memory_manager;

extern "C" void __cxa_pure_virtual() {
    while (1) __asm__("hlt");
}

int printk(const char format[], ...) {
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    console->PutString(s);
    return result;
}

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(
    const FrameBufferConfig &frame_buffer_config_ref,
    const MemoryMap &memory_map_ref) {
    FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    switch (frame_buffer_config.pixel_format) {
        case kPixelRGBResv8BitPerColor:
            screen_drawer = new (screen_drawer_buffer)
                RGB8BitScreenDrawer{frame_buffer_config};
            break;
        case kPixelBGRResv8BitPerColor:
            screen_drawer = new (screen_drawer_buffer)
                BGR8BitScreenDrawer{frame_buffer_config};
            break;
    }

    const int frame_width = frame_buffer_config.horizontal_resolution;
    const int frame_height = frame_buffer_config.vertical_resolution;

    FillRectangle(*screen_drawer, {0, 0}, {frame_width, frame_height - 50},
                  desktop_bg_color);

    FillRectangle(*screen_drawer, {0, frame_height - 50}, {frame_width, 50},
                  {80, 80, 80});

    console = new (console_buf)
        Console{*screen_drawer, {255, 255, 255}, {255, 255, 255}};

    SetupSegments();

    const uint16_t kernel_cs = 1 << 3;
    const uint16_t kernel_ss = 2 << 3;
    SetDSAll(0);
    SetCSSS(kernel_cs, kernel_ss);

    // メモリのページング設定
    SetupIdentityPageTable();

    ::memory_manager = new (memory_manager_buffer) BitmapMemoryManager;
    const auto memory_map_base_address =
        reinterpret_cast<uintptr_t>(memory_map.buffer);

    uintptr_t available_end = 0;
    for (uintptr_t iter = memory_map_base_address;
         iter < memory_map_base_address + memory_map.map_size;
         iter += memory_map.descriptor_size) {
        auto descriptor = reinterpret_cast<const MemoryDescriptor *>(iter);
        if (available_end < descriptor->physical_start) {
            memory_manager->MarkAllocated(
                FrameID{available_end / kBytesPerFrame},
                (descriptor->physical_start - available_end) / kBytesPerFrame);
        }

        const auto physical_end_address =
            descriptor->physical_start +
            descriptor->number_of_pages * kUEFIPageSize;

        if (IsAvailable(static_cast<MemoryType>(descriptor->type))) {
            available_end = physical_end_address;
        } else {
            memory_manager->MarkAllocated(
                FrameID{descriptor->physical_start / kBytesPerFrame},
                descriptor->number_of_pages * kUEFIPageSize / kBytesPerFrame);
        }
    }

    memory_manager->SetMemoryRange(FrameID{1},
                                   FrameID{available_end / kBytesPerFrame});

    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] == '@') {
                screen_drawer->Draw(200 + dx, 100 + dy, {0, 0, 0});
            } else if (mouse_cursor_shape[dy][dx] == '.') {
                screen_drawer->Draw(200 + dx, 100 + dy, {255, 255, 255});
            }
        }
    }

    while (1) __asm__("hlt");
}