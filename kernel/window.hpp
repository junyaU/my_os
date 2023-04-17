#pragma once

#include <optional>
#include <string>
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
    virtual ~Window() = default;
    Window(const Window& rhs) = delete;
    Window& operator=(const Window& rhs) = delete;

    void DrawTo(FrameBuffer& dst, Vector2D<int> position,
                const Rectangle<int>& area);
    void SetTransparentColor(std::optional<PixelColor> c);
    WindowDrawer* Drawer();

    const PixelColor& At(Vector2D<int> pos) const;

    void Draw(Vector2D<int> pos, PixelColor c);

    int Width() const;
    int Height() const;
    Vector2D<int> Size() const;

    void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

    virtual void Activate() {}
    virtual void Deactivate() {}

   private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowDrawer drawer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};

    FrameBuffer shadow_buffer_{};
};

class ToplevelWindow : public Window {
   public:
    static constexpr Vector2D<int> kTopLeftMargin{4, 24};
    static constexpr Vector2D<int> kBottomRightMargin{4, 4};

    class InnerAreaDrawer : public ScreenDrawer {
       public:
        InnerAreaDrawer(ToplevelWindow& window) : window_{window} {}
        virtual void Draw(Vector2D<int> pos, const PixelColor& c) override {
            window_.Draw(pos + kTopLeftMargin, c);
        }

        virtual int Width() const override {
            return window_.Width() - kTopLeftMargin.x - kBottomRightMargin.x;
        }

        virtual int Height() const override {
            return window_.Height() - kTopLeftMargin.y - kBottomRightMargin.y;
        }

       private:
        ToplevelWindow& window_;
    };

    ToplevelWindow(int width, int height, PixelFormat shadow_format,
                   const std::string& title);

    virtual void Activate() override;
    virtual void Deactivate() override;

    InnerAreaDrawer* InnerDrawer() { return &inner_drawer_; }
    Vector2D<int> InnerSize() const;

   private:
    std::string title_;
    InnerAreaDrawer inner_drawer_{*this};
};

void DrawWindow(ScreenDrawer& drawer, const char* title);
void DrawTextbox(ScreenDrawer& drawer, Vector2D<int> pos, Vector2D<int> size);
void DrawWindowTitle(ScreenDrawer& drawer, const char* title, bool active);