#include "console.hpp"

#include <cstring>

#include "font.hpp"

Console::Console(ScreenDrawer& drawer, const PixelColor& fg_color,
                 const PixelColor& bg_color)
    : drawer_{drawer},
      fg_color_{fg_color},
      bg_color_{bg_color},
      buffer_{},
      cursor_row_{0},
      cursor_column_{0} {}

void Console::PutString(const char* s) {
    while (*s) {
        if (*s == '\n') {
            NewLine();
            ++s;
            continue;
        }

        WriteAscii(drawer_, FONT_HORIZONTAL_PIXELS * cursor_column_,
                   FONT_VERTICAL_PIXELS * cursor_column_, *s, fg_color_);

        buffer_[cursor_row_][cursor_column_] = *s;
        ++cursor_column_;
        ++s;
    }
}

void Console::NewLine() {
    cursor_column_ = 0;
    if (cursor_row_ < rows - 1) {
        ++cursor_row_;
        return;
    }

    // refresh
    for (int y = 0; y < FONT_VERTICAL_PIXELS * rows; ++y) {
        for (int x = 0; x < FONT_HORIZONTAL_PIXELS * columuns; ++x) {
            drawer_.Draw(x, y, bg_color_);
        }
    }

    for (int row = 0; row < rows - 1; ++row) {
        // 1行下の列を現在の行にコピーする コピーした文字を出力
        memcpy(buffer_[row], buffer_[row + 1], columuns + 1);
        WriteString(drawer_, 0, FONT_VERTICAL_PIXELS * row, buffer_[row],
                    fg_color_);
    }

    memset(buffer_[rows - 1], 0, columuns + 1);
}