#include "console.hpp"

#include <cstring>

#include "font.hpp"
#include "layer.hpp"

Console::Console(const PixelColor& fg_color, const PixelColor& bg_color)
    : drawer_{nullptr},
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

        if (cursor_column_ < columuns - 1) {
            WriteAscii(*drawer_,
                       Vector2D<int>{FONT_HORIZONTAL_PIXELS * cursor_column_,
                                     FONT_VERTICAL_PIXELS * cursor_row_},
                       *s, fg_color_);

            buffer_[cursor_row_][cursor_column_] = *s;
            ++cursor_column_;
        }

        ++s;
    }

    if (layer_manager) {
        layer_manager->Draw();
    }
}

void Console::SetDrawer(ScreenDrawer* drawer) {
    if (drawer == drawer_) {
        return;
    }

    drawer_ = drawer;
    Refresh();
}

void Console::SetWindow(const std::shared_ptr<Window>& window) {
    if (window_ == window) {
        return;
    }

    window_ = window;
    drawer_ = window->Drawer();
    Refresh();
}

void Console::NewLine() {
    cursor_column_ = 0;
    if (cursor_row_ < rows - 1) {
        ++cursor_row_;
        return;
    }

    if (window_) {
    } else {
    }

    // // refresh
    // for (int y = 0; y < FONT_VERTICAL_PIXELS * rows; ++y) {
    //     for (int x = 0; x < FONT_HORIZONTAL_PIXELS * columuns; ++x) {
    //         drawer_->Draw(Vector2D<int>{x, y}, bg_color_);
    //     }
    // }

    // for (int row = 0; row < rows - 1; ++row) {
    //     // 1行下の列を現在の行にコピーする コピーした文字を出力
    //     memcpy(buffer_[row], buffer_[row + 1], columuns + 1);
    //     WriteString(*drawer_, Vector2D<int>{0, FONT_VERTICAL_PIXELS * row},
    //                 buffer_[row], fg_color_);
    // }

    // memset(buffer_[rows - 1], 0, columuns + 1);
}

void Console::Refresh() {
    for (int row = 0; row < rows; ++row) {
        WriteString(*drawer_, Vector2D<int>{0, FONT_VERTICAL_PIXELS * row},
                    buffer_[row], fg_color_);
    }
}