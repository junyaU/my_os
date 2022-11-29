#pragma once

#include <optional>
#include <vector>

#include "drawing.hpp"

class Window {
   public:
    class WindowDrawer : public ScreenDrawer {
       public:
        WindowDrawer(Window &window) : window_{window} {}
        virtual void Draw(int x, int y, const PixelColor &c) override {
            window_.At(x, y);
        }
        virtual int Width() const override { return window_.Width(); }
        virtual int Height() const override { return window_.Height(); }

       private:
        Window &window_;
    };

    Window(int width, int height);
    ~Window() = default;
    Window(const Window &rhs) = delete;
    Window &operator=(const Window &rhs) = delete;

    void DrawTo(ScreenDrawer &drawer, Vector2D<int> position);
    void SetTransparentColor(std::optional<PixelColor> c);
    WindowDrawer *Drawer();

    PixelColor &At(int x, int y);
    const PixelColor &At(int x, int y) const;

    int Width() const;
    int Height() const;

   private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowDrawer drawer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};
};