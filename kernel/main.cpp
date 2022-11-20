#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "asmfunc.h"
#include "console.hpp"
#include "drawing.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "segment.hpp"

// void *operator new(size_t size, void *buf) noexcept { return buf; }

void operator delete(void *obj) noexcept {}

const PixelColor desktop_bg_color{45, 118, 237};
const PixelColor desktop_fg_color{255, 255, 255};

const int mouse_cursor_width = 15;
const int mouse_cursor_height = 24;
const char mouse_cursor_shape[mouse_cursor_height][mouse_cursor_width + 1] = {
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
    // FillRectangle(*screen_drawer, {0, frame_height - 50}, {frame_width, 50},
    //               {1, 8, 17});
    FillRectangle(*screen_drawer, {0, frame_height - 50}, {frame_width, 50},
                  {80, 80, 80});

    console = new (console_buf)
        Console{*screen_drawer, {255, 255, 255}, {255, 255, 255}};

    SetupSegments();

    const uint16_t kernel_cs = 1 << 3;
    const uint16_t kernel_ss = 2 << 3;
    SetDSAll(0);
    SetCSSS(kernel_cs, kernel_ss);

    const std::array available_memory_types{
        MemoryType::kEfiBootServicesCode,
        MemoryType::kEfiBootServicesData,
        MemoryType::kEfiConventionalMemory,
    };

    printk("memory_map: %p\n", &memory_map);
    uintptr_t map_base_addr = reinterpret_cast<uintptr_t>(memory_map.buffer);
    for (uintptr_t iter = reinterpret_cast<uintptr_t>(memory_map.buffer);
         iter < map_base_addr + memory_map.map_size;
         iter += memory_map.descriptor_size) {
        auto descriptor = reinterpret_cast<MemoryDescriptor *>(iter);
        for (int i = 0; i < available_memory_types.size(); ++i) {
            if (descriptor->type == available_memory_types[i]) {
                printk(
                    "type = %u, phys = %08lx - %08lx, pages = %lu, attr = "
                    "%08lx\n",
                    descriptor->type, descriptor->physical_start,
                    descriptor->physical_start +
                        descriptor->number_of_pages * 4096 - 1,
                    descriptor->number_of_pages, descriptor->attribute);
            }
        }
    }

    while (1) __asm__("hlt");
}