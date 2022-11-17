#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "console.hpp"
#include "drawing.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "memory_map.hpp"

void *operator new(size_t size, void *buf) noexcept { return buf; }

void operator delete(void *obj) noexcept {}

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

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config,
                           const MemoryMap memory_map) {
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

    for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
        for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
            screen_drawer->Draw(x, y, {255, 255, 255});
        }
    }

    console =
        new (console_buf) Console{*screen_drawer, {0, 0, 0}, {255, 255, 255}};

    const std::array available_memory_types{
        MemoryType::kEfiBootServicesCode,
        MemoryType::kEfiBootServicesData,
        MemoryType::kEfiConventionalMemory,
    };

    printk("memory_map: %p\n", &memory_map);

    uintptr_t memory_map_base_address =
        reinterpret_cast<uintptr_t>(memory_map.buffer);

    for (; memory_map_base_address <
           memory_map_base_address + memory_map.map_size;
         memory_map_base_address += memory_map.descriptor_size) {
        auto descriptor =
            reinterpret_cast<MemoryDescriptor *>(memory_map_base_address);
        for (int i = 0; i < available_memory_types.size(); ++i) {
            // if (descriptor->type != available_memory_types[i]) {
            //     continue;
            // }

            printk("type = %u\n", descriptor->type);
        }
    }

    while (1) __asm__("hlt");
}