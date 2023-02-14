#pragma once

#include <cstdint>

#include "drawing.hpp"

#define FONT_HORIZONTAL_PIXELS 8
#define FONT_VERTICAL_PIXELS 16

void WriteAscii(ScreenDrawer& drawer, Vector2D<int> pos, char c,
                const PixelColor& color);
void WriteString(ScreenDrawer& screen_drawer, Vector2D<int> pos, const char s[],
                 const PixelColor& color);