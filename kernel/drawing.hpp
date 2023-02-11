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

class ScreenDrawer {
   public:
    virtual ~ScreenDrawer() = default;
    virtual void Draw(int x, int y, const PixelColor &c) = 0;
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
    uint8_t *PixelAt(int x, int y) {
        return config_.frame_buffer +
               4 * (config_.pixels_per_scan_line * y + x);
    }

   private:
    const FrameBufferConfig &config_;
};

class RGB8BitScreenDrawer : public FrameBufferDrawer {
   public:
    using FrameBufferDrawer::FrameBufferDrawer;

    virtual void Draw(int x, int y, const PixelColor &c) override;
};

class BGR8BitScreenDrawer : public FrameBufferDrawer {
   public:
    using FrameBufferDrawer::FrameBufferDrawer;

    virtual void Draw(int x, int y, const PixelColor &c) override;
};

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

void FillRectangle(ScreenDrawer &drawer, const Vector2D<int> &position,
                   const Vector2D<int> &area, const PixelColor &c);

void DrawRectangle(ScreenDrawer &drawer, const Vector2D<int> &position,
                   const Vector2D<int> &area, const PixelColor &c);

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

void DrawDesktop(ScreenDrawer &drawer);