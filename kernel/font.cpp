#include "font.hpp"

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

const uint8_t* GetFont(char c) {
    auto index = kFontVerticalPixels * static_cast<unsigned int>(c);
    if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
        return nullptr;
    }

    return &_binary_hankaku_bin_start + index;
}

void WriteAscii(ScreenDrawer& drawer, Vector2D<int> pos, char c,
                const PixelColor& color) {
    const uint8_t* font = GetFont(c);
    if (font == nullptr) {
        return;
    }

    for (int dy = 0; dy < kFontVerticalPixels; ++dy) {
        for (int dx = 0; dx < kFontHorizonPixels; ++dx) {
            if ((font[dy] << dx) & 0x80u) {
                drawer.Draw(pos + Vector2D<int>{dx, dy}, color);
            }
        }
    }
}

void WriteString(ScreenDrawer& drawer, Vector2D<int> pos, const char s[],
                 const PixelColor& color) {
    for (int i = 0; s[i] != '\0'; ++i) {
        WriteAscii(drawer, pos + Vector2D<int>{kFontHorizonPixels * i, 0}, s[i],
                   color);
    }
}