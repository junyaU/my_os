#include "drawing.hpp"

void RGB8BitScreenDrawer::Draw(int x, int y, const PixelColor& c) {
    auto target_pixel = PixelAt(x, y);
    target_pixel[0] = c.r;
    target_pixel[1] = c.g;
    target_pixel[2] = c.b;
}

void BGR8BitScreenDrawer::Draw(int x, int y, const PixelColor& c) {
    auto target_pixel = PixelAt(x, y);
    target_pixel[0] = c.b;
    target_pixel[1] = c.g;
    target_pixel[2] = c.r;
}

void FillRectangle(ScreenDrawer& drawer, const Vector2D<int>& position,
                   const Vector2D<int>& area, const PixelColor& c) {
    for (int dy = 0; dy < area.y; ++dy) {
        for (int dx = 0; dx < area.x; ++dx) {
            drawer.Draw(position.x + dx, position.y + dy, c);
        }
    }
}

void DrawRectangle(ScreenDrawer& drawer, const Vector2D<int>& position,
                   const Vector2D<int>& area, const PixelColor& c) {
    for (int dx = 0; dx < area.x; ++dx) {
        drawer.Draw(position.x + dx, position.y, c);
        drawer.Draw(position.x + dx, position.y + area.y - 1, c);
    }

    for (int dy = 1; dy < area.y - 1; ++dy) {
        drawer.Draw(position.x, position.y + dy, c);
        drawer.Draw(position.x + area.x - 1, position.y + dy, c);
    }
}