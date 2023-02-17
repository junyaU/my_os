#pragma once

#include <optional>
#include <vector>

#include "drawing.hpp"
#include "frame_buffer.hpp"

class Window {
   public:
    class WindowDrawer : public ScreenDrawer {
       public:
        WindowDrawer(Window& window) : window_{window} {}
        virtual void Draw(Vector2D<int> pos, const PixelColor& c) override {
            window_.Draw(pos, c);
        }

        virtual int Width() const override { return window_.Width(); }
        virtual int Height() const override { return window_.Height(); }

       private:
        Window& window_;
    };

    Window(int width, int height, PixelFormat shadow_format);
    ~Window() = default;
    Window(const Window& rhs) = delete;
    Window& operator=(const Window& rhs) = delete;

    void DrawTo(FrameBuffer& dst, Vector2D<int> position);
    void SetTransparentColor(std::optional<PixelColor> c);
    WindowDrawer* Drawer();

    const PixelColor& At(Vector2D<int> pos) const;

    void Draw(Vector2D<int> pos, PixelColor c);

    int Width() const;
    int Height() const;

    void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

   private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowDrawer drawer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};

    FrameBuffer shadow_buffer_{};
};