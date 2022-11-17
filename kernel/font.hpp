#pragma once

#include <cstdint>

#include "drawing.hpp"

#define FONT_HORIZONTAL_PIXELS 8
#define FONT_VERTICAL_PIXELS 16

void WriteAscii(ScreenDrawer& drawer, int x, int y, char c,
                const PixelColor& color);
void WriteString(ScreenDrawer& screen_drawer, int x, int y, const char s[],
                 const PixelColor& color);