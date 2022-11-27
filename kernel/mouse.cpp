#include "mouse.hpp"

#include "drawing.hpp"

namespace {
const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;

const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ", "@@             ", "@.@            ", "@..@           ",
    "@...@          ", "@....@         ", "@.....@        ", "@......@       ",
    "@.......@      ", "@........@     ", "@.........@    ", "@..........@   ",
    "@...........@  ", "@............@ ", "@......@@@@@@@@", "@......@       ",
    "@....@@.@      ", "@...@ @.@      ", "@..@   @.@     ", "@.@    @.@     ",
    "@@      @.@    ", "@       @.@    ", "         @.@   ", "         @@@   ",
};

void DrawMouseCursor(ScreenDrawer *drawer, Vector2D<int> position) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] == '@') {
                drawer->Draw(position.x + dx, position.y + dy, {0, 0, 0});
            } else if (mouse_cursor_shape[dy][dx] == '.') {
                drawer->Draw(position.x + dx, position.y + dy, {255, 255, 255});
            }
        }
    }
}

void EraseMouseCursor(ScreenDrawer *drawer, Vector2D<int> position,
                      PixelColor erase_color) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] != ' ') {
                drawer->Draw(position.x + dx, position.y + dy, erase_color);
            }
        }
    }
}

}  // namespace

MouseCursor::MouseCursor(ScreenDrawer *drawer, PixelColor erase_color,
                         Vector2D<int> initial_position)
    : screen_drawer_{drawer},
      erase_color_{erase_color},
      position_{initial_position} {
    DrawMouseCursor(screen_drawer_, position_);
}

void MouseCursor::MoveRelative(Vector2D<int> displacement) {
    EraseMouseCursor(screen_drawer_, position_, erase_color_);
    position_ += displacement;
    DrawMouseCursor(screen_drawer_, position_);
}