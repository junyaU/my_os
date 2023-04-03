#pragma once

#include <cstdint>

#include "drawing.hpp"

const int kFontHorizonPixels = 8;
const int kFontVerticalPixels = 16;

void WriteAscii(ScreenDrawer& drawer, Vector2D<int> pos, char c,
                const PixelColor& color);
void WriteString(ScreenDrawer& screen_drawer, Vector2D<int> pos, const char s[],
                 const PixelColor& color);