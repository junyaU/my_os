#include "window.hpp"

#include "font.hpp"
#include "logger.hpp"

namespace {
void DrawTextbox(ScreenDrawer& drawer, Vector2D<int> pos, Vector2D<int> size,
                 const PixelColor& background, const PixelColor& border_light,
                 const PixelColor& border_dark) {
    auto fill_rect = [&drawer](Vector2D<int> pos, Vector2D<int> size,
                               const PixelColor& color) {
        FillRectangle(drawer, pos, size, color);
    };

    fill_rect(pos + Vector2D<int>{1, 1}, size - Vector2D<int>{2, 2},
              background);

    fill_rect(pos, {size.x, 1}, border_dark);
    fill_rect(pos, {1, size.y}, border_dark);
    fill_rect(pos + Vector2D<int>{0, size.y}, {size.x, 1}, border_light);
    fill_rect(pos + Vector2D<int>{size.x, 0}, {1, size.y}, border_light);
}
}  // namespace

Window::Window(int width, int height, PixelFormat shadow_format)
    : width_{width}, height_{height} {
    data_.resize(height);
    for (int y = 0; y < height; ++y) {
        data_[y].resize(width);
    }

    FrameBufferConfig config{};
    config.frame_buffer = nullptr;
    config.horizontal_resolution = width;
    config.vertical_resolution = height;
    config.pixel_format = shadow_format;

    if (auto err = shadow_buffer_.Initialize(config)) {
        Log(kError, "failed to initialize shadow buffer: %s at %s:%d\n",
            err.Name(), err.File(), err.Line());
    }
}

void Window::DrawTo(FrameBuffer& dst, Vector2D<int> position,
                    const Rectangle<int>& area) {
    if (!transparent_color_) {
        Rectangle<int> window_area{position, Size()};
        Rectangle<int> intersection = area & window_area;
        dst.Copy(intersection.pos, shadow_buffer_,
                 {intersection.pos - position, intersection.size});
        return;
    }

    const auto tc = transparent_color_.value();
    auto& drawer = dst.Drawer();

    for (int y = 0; y < std::min(Height(), drawer.Height() - position.y); ++y) {
        for (int x = std::max(0, 0 - position.x);
             x < std::min(Width(), drawer.Width() - position.x); ++x) {
            const auto c = At(Vector2D<int>{x, y});
            if (c != tc) {
                drawer.Draw(position + Vector2D<int>{x, y}, c);
            }
        }
    }
}

void Window::SetTransparentColor(std::optional<PixelColor> c) {
    transparent_color_ = c;
}

Window::WindowDrawer* Window::Drawer() { return &drawer_; }

void Window::Draw(Vector2D<int> pos, PixelColor c) {
    data_[pos.y][pos.x] = c;
    shadow_buffer_.Drawer().Draw(pos, c);
}

const PixelColor& Window::At(Vector2D<int> pos) const {
    return data_[pos.y][pos.x];
}

int Window::Width() const { return width_; }

int Window::Height() const { return height_; }

Vector2D<int> Window::Size() const { return {width_, height_}; }

void Window::Move(Vector2D<int> dst_pos, const Rectangle<int>& src) {
    shadow_buffer_.Move(dst_pos, src);
}

ToplevelWindow::ToplevelWindow(int width, int height, PixelFormat shadow_format,
                               const std::string& title)
    : Window{width, height, shadow_format}, title_{title} {
    DrawWindow(*Drawer(), title_.c_str());
}

void ToplevelWindow::Activate() {
    Window::Activate();
    DrawWindowTitle(*Drawer(), title_.c_str(), true);
}

void ToplevelWindow::Deactivate() {
    Window::Deactivate();
    DrawWindowTitle(*Drawer(), title_.c_str(), false);
}

Vector2D<int> ToplevelWindow::InnerSize() const {
    return Size() - kTopLeftMargin - kBottomRightMargin;
}

namespace {
const int kCloseButtonWidth = 16;
const int kCloseButtonHeight = 14;
const char close_button[kCloseButtonHeight][kCloseButtonWidth + 1] = {
    "...............@", ".:::::::::::::$@", ".:::::::::::::$@",
    ".:::@@::::@@::$@", ".::::@@::@@:::$@", ".:::::@@@@::::$@",
    ".::::::@@:::::$@", ".:::::@@@@::::$@", ".::::@@::@@:::$@",
    ".:::@@::::@@::$@", ".:::::::::::::$@", ".:::::::::::::$@",
    ".$$$$$$$$$$$$$$@", "@@@@@@@@@@@@@@@@",
};

}  // namespace

void DrawWindow(ScreenDrawer& drawer, const char* title) {
    auto fill_rect = [&drawer](Vector2D<int> pos, Vector2D<int> size,
                               uint32_t c) {
        FillRectangle(drawer, pos, size, ToColor(c));
    };
    const auto win_w = drawer.Width();
    const auto win_h = drawer.Height();

    fill_rect({0, 0}, {win_w, 1}, 0xc6c6c6);
    fill_rect({1, 1}, {win_w - 2, 1}, 0xffffff);
    fill_rect({0, 0}, {1, win_h}, 0xc6c6c6);
    fill_rect({1, 1}, {1, win_h - 2}, 0xffffff);
    fill_rect({win_w - 2, 1}, {1, win_h - 2}, 0x848484);
    fill_rect({win_w - 1, 0}, {1, win_h}, 0x000000);
    fill_rect({2, 2}, {win_w - 4, win_h - 4}, 0xc6c6c6);
    fill_rect({1, win_h - 2}, {win_w - 2, 1}, 0x848484);
    fill_rect({0, win_h - 1}, {win_w, 1}, 0x000000);

    DrawWindowTitle(drawer, title, false);
}

void DrawTextbox(ScreenDrawer& drawer, Vector2D<int> pos, Vector2D<int> size) {
    DrawTextbox(drawer, pos, size, ToColor(0xffffff), ToColor(0xc6c6c6),
                ToColor(0x848484));
}

void DrawTerminal(ScreenDrawer& drawer, Vector2D<int> pos, Vector2D<int> size) {
    DrawTextbox(drawer, pos, size, ToColor(0x000000), ToColor(0xc6c6c6),
                ToColor(0x848484));
}

void DrawWindowTitle(ScreenDrawer& drawer, const char* title, bool active) {
    const auto window_width = drawer.Width();
    uint32_t bgcolor = 0x848484;
    if (active) {
        bgcolor = 0x000084;
    }

    FillRectangle(drawer, {3, 3}, {window_width - 6, 18}, ToColor(bgcolor));
    WriteString(drawer, {24, 4}, title, ToColor(0xffffff));

    for (int y = 0; y < kCloseButtonHeight; ++y) {
        for (int x = 0; x < kCloseButtonWidth; ++x) {
            PixelColor c = ToColor(0xffffff);

            if (close_button[y][x] == '@') {
                c = ToColor(0x000000);
            } else if (close_button[y][x] == '$') {
                c = ToColor(0x848484);
            } else if (close_button[y][x] == ':') {
                c = ToColor(0xc6c6c6);
            }

            drawer.Draw({window_width - 5 - kCloseButtonWidth + x, 5 + y}, c);
        }
    }
}