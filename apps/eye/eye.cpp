#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "../syscall.h"

static const int kCanvasSize = 100, kEyeSize = 10;

void DrawEye(uint64_t layer_id_flags, int mouse_x, int mouse_y,
             uint32_t color) {
    // 中心からの相対座標
    const double center_x = mouse_x - kCanvasSize / 2 - 4;
    const double center_y = mouse_y - kCanvasSize / 2 - 24;

    // 中心から見て、どの方向（角度）にあるかを計算
    const double direction = atan2(center_y, center_x);
    // ピタゴラスの定理で、中心からの距離を計算
    double distance = sqrt(pow(center_x, 2.0) + pow(center_y, 2.0));
    distance = std::min<double>(distance, kCanvasSize / 2 - kEyeSize / 2);

    const double eye_center_x = cos(direction) * distance;
    const double eye_center_y = sin(direction) * distance;
    const int eye_x = static_cast<int>(eye_center_x) + kCanvasSize / 2 + 4;
    const int eye_y = static_cast<int>(eye_center_y) + kCanvasSize / 2 + 24;

    SyscallWinFillRectangle(layer_id_flags, eye_x - kEyeSize / 2,
                            eye_y - kEyeSize / 2, kEyeSize, kEyeSize, color);
}

extern "C" void main(int argc, char** argv) {
    auto [layer_id, err] =
        SyscallOpenWindow(kCanvasSize + 8, kCanvasSize + 28, 10, 10, "eye");
    if (err) {
        exit(err);
    }

    SyscallWinFillRectangle(layer_id, 4, 24, kCanvasSize, kCanvasSize,
                            0xffffff);

    AppEvent events[1];
    while (true) {
        auto [n, err] = SyscallReadEvent(events, 1);
        if (err) {
            printf("ReadEvent failed: %s\n", strerror(err));
            break;
        }

        if (events[0].type == AppEvent::kQuit) {
            break;
        }

        if (events[0].type == AppEvent::kMouseMove) {
            auto& arg = events[0].arg.mouse_move;
            SyscallWinFillRectangle(layer_id | LAYER_NO_REDRAW, 4, 24,
                                    kCanvasSize, kCanvasSize, 0xffffff);
            DrawEye(layer_id, arg.x, arg.y, 0x000000);
        } else {
            printf("unknown event: type = %d\n", events[0].type);
        }
    }

    SyscallCloseWindow(layer_id);
    exit(0);
}