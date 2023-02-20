#include "window.hpp"

#include "font.hpp"
#include "logger.hpp"

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

void Window::DrawTo(FrameBuffer& dst, Vector2D<int> position) {
    if (!transparent_color_) {
        dst.Copy(position, shadow_buffer_);
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

void Window::Move(Vector2D<int> dst_pos, const Rectangle<int>& src) {
    shadow_buffer_.Move(dst_pos, src);
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

constexpr PixelColor ToColor(uint32_t c) {
    return {static_cast<uint8_t>((c >> 16) & 0xff),
            static_cast<uint8_t>((c >> 8) & 0xff),
            static_cast<uint8_t>(c & 0xff)};
}
}  // namespace

void DrawWindow(ScreenDrawer& drawer, const char* title) {
    auto fill_rect = [&drawer](Vector2D<int> pos, Vector2D<int> size,
                               uint32_t color) {
        FillRectangle(drawer, pos, size, ToColor(color));
    };

    const auto window_width = drawer.Width();
    const auto window_height = drawer.Height();

    fill_rect({0, 0}, {window_width, 1}, 0xc6c6c6);
    fill_rect({1, 1}, {window_width - 2, 1}, 0xffffff);
    fill_rect({0, 0}, {1, window_height}, 0xc6c6c6);
    fill_rect({1, 1}, {1, window_height - 2}, 0xffffff);
    fill_rect({window_width - 2, 1}, {1, window_height - 2}, 0x848484);
    fill_rect({window_width - 1, 0}, {1, window_height}, 0x000000);
    fill_rect({2, 2}, {window_width - 4, window_height - 4}, 0xc6c6c6);
    fill_rect({3, 3}, {window_width - 6, 18}, 0x000084);
    fill_rect({1, window_height - 2}, {window_width - 2, 1}, 0x848484);
    fill_rect({0, window_height - 1}, {window_width, 1}, 0x000000);

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