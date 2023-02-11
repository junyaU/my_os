#include "mouse.hpp"

#include "drawing.hpp"

namespace {
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ", "@@             ", "@.@            ", "@..@           ",
    "@...@          ", "@....@         ", "@.....@        ", "@......@       ",
    "@.......@      ", "@........@     ", "@.........@    ", "@..........@   ",
    "@...........@  ", "@............@ ", "@......@@@@@@@@", "@......@       ",
    "@....@@.@      ", "@...@ @.@      ", "@..@   @.@     ", "@.@    @.@     ",
    "@@      @.@    ", "@       @.@    ", "         @.@   ", "         @@@   ",
};
}  // namespace

void DrawMouseCursor(ScreenDrawer *drawer, Vector2D<int> position) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] == '@') {
                drawer->Draw(position.x + dx, position.y + dy, {0, 0, 0});
            } else if (mouse_cursor_shape[dy][dx] == '.') {
                drawer->Draw(position.x + dx, position.y + dy, {255, 255, 255});
            } else {
                drawer->Draw(position.x + dx, position.y + dy,
                             kMouseTransparentColor);
            }
        }
    }
}
