#pragma once
#include "frame_buffer_config.hpp"

struct PixelColor {
    uint8_t r, g, b;
};

class ScreenDrawer {
   public:
    ScreenDrawer(const FrameBufferConfig &config) : config_{config} {}
    virtual ~ScreenDrawer() = default;
    virtual void Draw(int x, int y, const PixelColor &c) = 0;
    virtual int Width() const = 0;
    virtual int Height() const = 0;

    //　該当ピクセルのアドレス
   protected:
    uint8_t *PixelAt(int x, int y) {
        return config_.frame_buffer +
               4 * (config_.pixels_per_scan_line * y + x);
    }

   private:
    const FrameBufferConfig &config_;
};

class RGB8BitScreenDrawer : public ScreenDrawer {
   public:
    using ScreenDrawer::ScreenDrawer;

    virtual void Draw(int x, int y, const PixelColor &c) override;
};

class BGR8BitScreenDrawer : public ScreenDrawer {
   public:
    using ScreenDrawer::ScreenDrawer;

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
