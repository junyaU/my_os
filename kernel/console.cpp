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

        if (cursor_column_ < kColumns - 1) {
            WriteAscii(*drawer_,
                       Vector2D<int>{kFontHorizonPixels * cursor_column_,
                                     kFontVerticalPixels * cursor_row_},
                       *s, fg_color_);

            buffer_[cursor_row_][cursor_column_] = *s;
            ++cursor_column_;
        }

        ++s;
    }

    if (layer_manager) {
        layer_manager->Draw(layer_id_);
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

void Console::SetLayerID(unsigned int layer_id) { layer_id_ = layer_id; }

unsigned int Console::LayerID() const { return layer_id_; }

void Console::NewLine() {
    cursor_column_ = 0;
    if (cursor_row_ < kRows - 1) {
        ++cursor_row_;
        return;
    }

    if (window_) {
        Rectangle<int> move_src{
            {0, kFontVerticalPixels},
            {kFontHorizonPixels * kColumns, kFontVerticalPixels * (kRows - 1)}};
        window_->Move({0, 0}, move_src);
        FillRectangle(*drawer_, {0, kFontVerticalPixels * (kRows - 1)},
                      {kFontHorizonPixels * kColumns, kFontVerticalPixels},
                      bg_color_);
    } else {
        FillRectangle(
            *drawer_, {0, 0},
            {kFontHorizonPixels * kColumns, kFontVerticalPixels * (kRows - 1)},
            bg_color_);
        for (int row = 0; row < kRows; row++) {
            memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
            WriteString(*drawer_, Vector2D<int>{0, kFontVerticalPixels * row},
                        buffer_[row], fg_color_);
        }
        memset(buffer_[kRows - 1], 0, kColumns + 1);
    }
}

void Console::Refresh() {
    for (int row = 0; row < kRows; ++row) {
        WriteString(*drawer_, Vector2D<int>{0, kFontVerticalPixels * row},
                    buffer_[row], fg_color_);
    }
}

Console* console;

namespace {
char console_buf[sizeof(Console)];
}

void InitializeConsole() {
    console = new (console_buf) Console{kDesktopFGColor, kDesktopBGColor};
    console->SetDrawer(screen_drawer);
}

int printk(const char format[], ...) {
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    console->PutString(s);
    return result;
}