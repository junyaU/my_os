#pragma once

#include <memory>
#include <vector>

#include "drawing.hpp"
#include "error.hpp"
#include "frame_buffer_config.hpp"

class FrameBuffer {
   public:
    Error Initialize(const FrameBufferConfig& config);
    Error Copy(Vector2D<int> pos, const FrameBuffer& src,
               const Rectangle<int>& src_area);
    void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

    FrameBufferDrawer& Drawer() { return *drawer_; }
    const FrameBufferConfig& Config() const { return config_; }

   private:
    FrameBufferConfig config_{};
    std::vector<uint8_t> buffer_{};
    std::unique_ptr<FrameBufferDrawer> drawer_{};
};

int BitsPerPixel(PixelFormat format);