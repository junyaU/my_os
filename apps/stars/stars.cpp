#include <cstdlib>
#include <random>

#include "../syscall.h"

static constexpr int kWidth = 100, kHeight = 100;

extern "C" void main(int argc, char** argv) {
    auto [layer_id, err] =
        SyscallOpenWindow(kWidth + 8, kHeight + 28, 10, 10, "stars");
    if (err) {
        exit(err);
    }

    SyscallWinFillRectangle(layer_id | LAYER_NO_REDRAW, 4, 24, kWidth, kHeight,
                            0x000000);

    int num_stars = 100;
    if (argc >= 2) {
        num_stars = atoi(argv[1]);
    }

    auto [tick, freq] = SyscallGetCurrentTick();

    std::default_random_engine engine;
    std::uniform_int_distribution x_dist(0, kWidth - 2), y_dist(0, kHeight - 2);

    for (int i = 0; i < num_stars; ++i) {
        const int x = x_dist(engine), y = y_dist(engine);
        SyscallWinFillRectangle(layer_id | LAYER_NO_REDRAW, 4 + x, 24 + y, 2, 2,
                                0xffffff);
    }

    SyscallWinRedraw(layer_id);

    auto tick_end = SyscallGetCurrentTick();
    printf("%d stars in %lu ms.\n", num_stars,
           (tick_end.value - tick) * 1000 / freq);

    exit(0);
}