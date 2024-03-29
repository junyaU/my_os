#pragma once

#include <ft2build.h>

#include <cstdint>
#include <utility>

#include "error.hpp"
#include FT_FREETYPE_H

#include "drawing.hpp"

const int kFontHorizonPixels = 8;
const int kFontVerticalPixels = 16;

void WriteAscii(ScreenDrawer& drawer, Vector2D<int> pos, char c,
                const PixelColor& color);
void WriteString(ScreenDrawer& screen_drawer, Vector2D<int> pos, const char s[],
                 const PixelColor& color);

int CountUTF8Size(uint8_t c);
std::pair<char32_t, int> ConvertUTF8To32(const char* u8);
bool IsHankaku(char32_t c);
Error WriteUnicode(ScreenDrawer& drawer, Vector2D<int> pos, char32_t c,
                   const PixelColor& color);
void InitializeFont();
