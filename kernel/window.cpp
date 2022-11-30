#include "window.hpp"

Window::Window(int width, int height) : width_{width}, height_{height} {
    data_.resize(height);
    for (int y = 0; y < height; ++y) {
        data_[y].resize(width);
    }
}

void Window::DrawTo(ScreenDrawer &drawer, Vector2D<int> position) {
    if (!transparent_color_) {
        for (int y = 0; y < Height(); ++y) {
            for (int x = 0; x < Width(); ++x) {
                drawer.Draw(x + position.x, y + position.y, At(x, y));
            }
        }
        return;
    }

    const auto tc = transparent_color_.value();
    for (int y = 0; y < Height(); ++y) {
        for (int x = 0; x < Width(); ++x) {
            const auto color = At(x, y);
            if (color != tc) {
                drawer.Draw(position.x + x, position.y + y, c);
            }
        }
    }
}

void Window::SetTransparentColor(std::optional<PixelColor> c) {
    transparent_color_ = c;
}