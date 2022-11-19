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

// const mouse_cursor_shape

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

    const int frame_width = frame_buffer_config.horizontal_resolution;
    const int frame_height = frame_buffer_config.vertical_resolution;

    FillRectangle(*screen_drawer, {0, 0}, {frame_width, frame_height - 50},
                  desktop_bg_color);
    FillRectangle(*screen_drawer, {0, frame_height - 50}, {frame_width, 50},
                  {1, 8, 17});
    FillRectangle(*screen_drawer, {0, frame_height - 50}, {frame_width / 5, 50},
                  {80, 80, 80});
    DrawRectangle(*screen_drawer, {10, frame_height - 40}, {30, 30},
                  {160, 160, 160});

    console =
        new (console_buf) Console{*screen_drawer, {0, 0, 0}, {255, 255, 255}};

    printk("printk: %s\n", "yorodesu");

    for (int dy = 0; dy < mouse_cursor_height; ++dy) {
        for (int dx = 0; dx < mouse_cursor_width; ++dx) {
            if (mouse_cursor_shape[dy][dx] == '@') {
                screen_drawer->Draw(200 + dx, 100 + dy, {0, 0, 0});
            } else if (mouse_cursor_shape[dy][dx] == '.') {
                screen_drawer->Draw(200 + dx, 100 + dy, {255, 255, 255});
            }
        }
    }

    while (1) __asm__("hlt");
}