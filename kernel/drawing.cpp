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