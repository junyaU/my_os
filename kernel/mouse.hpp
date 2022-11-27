#pragma once

#include "drawing.hpp"

class MouseCursor {
   public:
    MouseCursor(ScreenDrawer* drawer, PixelColor erase_color,
                Vector2D<int> initial_position);
    void MoveRelative(Vector2D<int> displacement);

   private:
    ScreenDrawer* screen_drawer_ = nullptr;
    PixelColor erase_color_;
    Vector2D<int> position_;
};