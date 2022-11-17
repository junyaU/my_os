#pragma once

#include "drawing.hpp"

class Console {
   public:
    static const int rows = 25, columuns = 80;

    Console(ScreenDrawer& drawer, const PixelColor& fg_color,
            const PixelColor& bg_color);
    void PutString(const char* s);

   private:
    void NewLine();

    ScreenDrawer& drawer_;
    const PixelColor fg_color_, bg_color_;
    char buffer_[rows][columuns + 1];
    int cursor_row_, cursor_column_;
};