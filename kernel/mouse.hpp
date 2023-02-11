#pragma once

#include "drawing.hpp"

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const PixelColor kMouseTransparentColor{0, 0, 1};

void DrawMouseCursor(ScreenDrawer* screen_drawer, Vector2D<int> position);