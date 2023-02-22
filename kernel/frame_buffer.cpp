#include "frame_buffer.hpp"

namespace {
int BytesPerPixels(PixelFormat format) {
    switch (format) {
        case kPixelBGRResv8BitPerColor:
            return 4;
        case kPixelRGBResv8BitPerColor:
            return 4;
        default:
            return -1;
    }
}

uint8_t* FrameAddrAt(Vector2D<int> pos, const FrameBufferConfig& config) {
    return config.frame_buffer +
           BytesPerPixels(config.pixel_format) *
               (config.pixels_per_scan_line * pos.y + pos.x);
}

int BytesPerScanLine(const FrameBufferConfig& config) {
    return config.pixels_per_scan_line * BytesPerPixels(config.pixel_format);
}

Vector2D<int> FrameBufferSize(const FrameBufferConfig& config) {
    return {static_cast<int>(config.horizontal_resolution),
            static_cast<int>(config.vertical_resolution)};
}
}  // namespace

Error FrameBuffer::Initialize(const FrameBufferConfig& config) {
    config_ = config;

    const auto bytes_per_pixel = BytesPerPixels(config_.pixel_format);
    if (bytes_per_pixel <= 0) {
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    if (config_.frame_buffer) {
        buffer_.resize(0);
    } else {
        buffer_.resize(bytes_per_pixel * config_.horizontal_resolution *
                       config_.vertical_resolution);
        config_.frame_buffer = buffer_.data();
        config_.pixels_per_scan_line = config_.horizontal_resolution;
    }

    switch (config_.pixel_format) {
        case kPixelRGBResv8BitPerColor:
            drawer_ = std::make_unique<RGB8BitScreenDrawer>(config_);
            break;
        case kPixelBGRResv8BitPerColor:
            drawer_ = std::make_unique<BGR8BitScreenDrawer>(config_);
            break;
        default:
            return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    return MAKE_ERROR(Error::kSuccess);
}

Error FrameBuffer::Copy(Vector2D<int> dst_pos, const FrameBuffer& src,
                        const Rectangle<int>& src_area) {
    if (config_.pixel_format != src.config_.pixel_format) {
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    const auto bytes_per_pixel = BytesPerPixels(config_.pixel_format);
    if (bytes_per_pixel <= 0) {
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    const Rectangle<int> src_area_shifted{dst_pos, src_area.size};
    const Rectangle<int> src_outline{dst_pos - src_area.pos,
                                     FrameBufferSize(src.config_)};
    const Rectangle<int> dst_outline{{0, 0}, FrameBufferSize(config_)};
    const auto copy_area = dst_outline & src_outline & src_area_shifted;
    const auto src_start_pos = copy_area.pos - (dst_pos - src_area.pos);

    uint8_t* dst_buf = FrameAddrAt(copy_area.pos, config_);
    const uint8_t* src_buf = FrameAddrAt(src_start_pos, src.config_);

    for (int y = 0; y < copy_area.size.y; ++y) {
        memcpy(dst_buf, src_buf, bytes_per_pixel * copy_area.size.x);
        dst_buf += BytesPerScanLine(config_);
        src_buf += BytesPerScanLine(src.config_);
    }

    return MAKE_ERROR(Error::kSuccess);
}

void FrameBuffer::Move(Vector2D<int> dst_pos, const Rectangle<int>& src) {
    const auto bytes_per_pixel = BytesPerPixels(config_.pixel_format);
    const auto bytes_per_scan_line = BytesPerScanLine(config_);

    if (dst_pos.y < src.pos.y) {
        uint8_t* dst_buf = FrameAddrAt(dst_pos, config_);
        const uint8_t* src_buf = FrameAddrAt(src.pos, config_);

        for (int y = 0; y < src.size.y; ++y) {
            memcpy(dst_buf, src_buf, bytes_per_pixel * src.size.x);
            dst_buf += bytes_per_scan_line;
            src_buf += bytes_per_scan_line;
        }
    } else {
        uint8_t* dst_buf =
            FrameAddrAt(dst_pos + Vector2D<int>{0, src.size.y - 1}, config_);
        const uint8_t* src_buf =
            FrameAddrAt(src.pos + Vector2D<int>{0, src.size.y - 1}, config_);

        for (int y = 0; y < src.size.y; ++y) {
            memcpy(dst_buf, src_buf, bytes_per_pixel * src.size.x);
            dst_buf -= bytes_per_scan_line;
            src_buf -= bytes_per_scan_line;
        }
    }
}
