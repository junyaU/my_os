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

void DrawMouseCursor(ScreenDrawer *drawer, Vector2D<int> pos) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] == '@') {
                drawer->Draw(pos, {0, 0, 0});
            } else if (mouse_cursor_shape[dy][dx] == '.') {
                drawer->Draw(pos + Vector2D<int>{dx, dy}, {255, 255, 255});
            } else {
                drawer->Draw(pos + Vector2D<int>{dx, dy},
                             kMouseTransparentColor);
            }
        }
    }
}
