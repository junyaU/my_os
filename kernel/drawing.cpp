#include "drawing.hpp"

void RGB8BitScreenDrawer::Draw(Vector2D<int> pos, const PixelColor& c) {
    auto target_pixel = PixelAt(pos);
    target_pixel[0] = c.r;
    target_pixel[1] = c.g;
    target_pixel[2] = c.b;
}

void BGR8BitScreenDrawer::Draw(Vector2D<int> pos, const PixelColor& c) {
    auto target_pixel = PixelAt(pos);
    target_pixel[0] = c.b;
    target_pixel[1] = c.g;
    target_pixel[2] = c.r;
}

void FillRectangle(ScreenDrawer& drawer, const Vector2D<int>& pos,
                   const Vector2D<int>& area, const PixelColor& c) {
    for (int dy = 0; dy < area.y; ++dy) {
        for (int dx = 0; dx < area.x; ++dx) {
            drawer.Draw(pos + Vector2D<int>{dx, dy}, c);
        }
    }
}

void DrawRectangle(ScreenDrawer& drawer, const Vector2D<int>& pos,
                   const Vector2D<int>& area, const PixelColor& c) {
    for (int dx = 0; dx < area.x; ++dx) {
        drawer.Draw(pos + Vector2D<int>{dx, 0}, c);
        drawer.Draw(pos + Vector2D<int>{0, area.y - 1}, c);
    }

    for (int dy = 1; dy < area.y - 1; ++dy) {
        drawer.Draw(pos + Vector2D<int>{0, dy}, c);
        drawer.Draw(pos + Vector2D<int>{area.x - 1, dy}, c);
    }
}

void DrawDesktop(ScreenDrawer& drawer) {
    const auto width = drawer.Width();
    const auto height = drawer.Height();
    FillRectangle(drawer, {0, 0}, {width, height - 50}, kDesktopBGColor);
    FillRectangle(drawer, {0, height - 50}, {width, 50}, {1, 8, 17});
    FillRectangle(drawer, {0, height - 50}, {width / 5, 50}, {80, 80, 80});
    DrawRectangle(drawer, {10, height - 40}, {30, 30}, {160, 160, 160});
}

FrameBufferConfig screen_config;
ScreenDrawer* screen_drawer;

Vector2D<int> ScreenSize() {
    return {static_cast<int>(screen_config.horizontal_resolution),
            static_cast<int>(screen_config.vertical_resolution)};
}

namespace {
char screen_drawer_buf[sizeof(RGB8BitScreenDrawer)];
}

void InitializeDrawings(const FrameBufferConfig& screen_config) {
    ::screen_config = screen_config;

    switch (screen_config.pixel_format) {
        case kPixelRGBResv8BitPerColor:
            ::screen_drawer =
                new (screen_drawer_buf) RGB8BitScreenDrawer{screen_config};
            break;
        case kPixelBGRResv8BitPerColor:
            ::screen_drawer =
                new (screen_drawer_buf) BGR8BitScreenDrawer{screen_config};
            break;
        default:
            exit(1);
    }

    DrawDesktop(*screen_drawer);
}