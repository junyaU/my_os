#include "font.hpp"

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

const uint8_t* GetFont(char c) {
    auto index = FONT_VERTICAL_PIXELS * static_cast<unsigned int>(c);
    if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
        return nullptr;
    }

    return &_binary_hankaku_bin_start + index;
}

void WriteAscii(ScreenDrawer& drawer, int x, int y, char c,
                const PixelColor& color) {
    const uint8_t* font = GetFont(c);
    if (font == nullptr) {
        return;
    }

    for (int dy = 0; dy < FONT_VERTICAL_PIXELS; ++dy) {
        for (int dx = 0; dx < FONT_HORIZONTAL_PIXELS; ++dx) {
            if ((font[dy] << dx) & 0x80u) {
                drawer.Draw(x + dx, y + dy, color);
            }
        }
    }
}

void WriteString(ScreenDrawer& drawer, int x, int y, const char s[],
                 const PixelColor& color) {
    for (int i = 0; s[i] != '\0'; ++i) {
        WriteAscii(drawer, x + FONT_HORIZONTAL_PIXELS * i, y, s[i], color);
    }
}