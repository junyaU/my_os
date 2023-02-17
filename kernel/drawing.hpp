#pragma once
#include "frame_buffer_config.hpp"

struct PixelColor {
    uint8_t r, g, b;
};

inline bool operator==(const PixelColor &lhs, const PixelColor &rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

inline bool operator!=(const PixelColor &lhs, const PixelColor &rhs) {
    return !(lhs == rhs);
}

template <typename T>
struct Vector2D {
    T x, y;

    template <typename U>
    Vector2D<T> &operator+=(const Vector2D<U> &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
};

template <typename T, typename U>
auto operator+(const Vector2D<T> &lhs, const Vector2D<U> &rhs)
    -> Vector2D<decltype(lhs.x + rhs.x)> {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

template <typename T>
Vector2D<T> ElementMax(const Vector2D<T> &lhs, const Vector2D<T> rhs) {
    return {std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y)};
}

template <typename T>
Vector2D<T> ElementMin(const Vector2D<T> &lhs, const Vector2D<T> rhs) {
    return {std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y)};
}

template <typename T>
struct Rectangle {
    Vector2D<T> pos, size;
};

class ScreenDrawer {
   public:
    virtual ~ScreenDrawer() = default;

    virtual void Draw(Vector2D<int> pos, const PixelColor &c) = 0;
    virtual int Width() const = 0;
    virtual int Height() const = 0;
};

class FrameBufferDrawer : public ScreenDrawer {
   public:
    FrameBufferDrawer(const FrameBufferConfig &config) : config_{config} {}
    virtual ~FrameBufferDrawer() = default;
    virtual int Width() const override { return config_.horizontal_resolution; }
    virtual int Height() const override { return config_.vertical_resolution; }

   protected:
    uint8_t *PixelAt(Vector2D<int> pos) {
        return config_.frame_buffer +
               4 * (config_.pixels_per_scan_line * pos.y + pos.x);
    }

   private:
    const FrameBufferConfig &config_;
};

class RGB8BitScreenDrawer : public FrameBufferDrawer {
   public:
    using FrameBufferDrawer::FrameBufferDrawer;

    virtual void Draw(Vector2D<int> pos, const PixelColor &c) override;
};

class BGR8BitScreenDrawer : public FrameBufferDrawer {
   public:
    using FrameBufferDrawer::FrameBufferDrawer;

    virtual void Draw(Vector2D<int> pos, const PixelColor &c) override;
};

void FillRectangle(ScreenDrawer &drawer, const Vector2D<int> &position,
                   const Vector2D<int> &area, const PixelColor &c);

void DrawRectangle(ScreenDrawer &drawer, const Vector2D<int> &position,
                   const Vector2D<int> &area, const PixelColor &c);

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

void DrawDesktop(ScreenDrawer &drawer);