#include "syscall.hpp"

#include <array>
#include <cerrno>
#include <cstdint>

#include "asmfunc.h"
#include "console.hpp"
#include "font.hpp"
#include "msr.hpp"
#include "task.hpp"
#include "terminal.hpp"

namespace syscall {
struct Result {
    uint64_t value;
    int error;
};

#define SYSCALL(name)                                                       \
    Result name(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, \
                uint64_t arg5, uint64_t arg6)

SYSCALL(LogString) {
    const char *s = reinterpret_cast<const char *>(arg1);

    const auto len = strlen(s);
    if (len > 1024) {
        return {0, E2BIG};
    }

    printk("%s", s);

    return {len, 0};
}

SYSCALL(PutString) {
    const auto fd = arg1;
    const char *s = reinterpret_cast<const char *>(arg2);
    const auto len = arg3;

    if (len > 1024) {
        return {0, E2BIG};
    }

    if (fd == 1) {
        const auto task_id = task_manager->CurrentTask().ID();
        (*terminals)[task_id]->Print(s, len);
        return {len, 0};
    }

    return {0, EBADF};
}

SYSCALL(Exit) {
    __asm__("cli");
    auto &task = task_manager->CurrentTask();
    __asm__("sti");
    return {task.OSStackPointer(), static_cast<int>(arg1)};
}

SYSCALL(OpenWindow) {
    const int w = arg1, h = arg2, x = arg3, y = arg4;
    const auto title = reinterpret_cast<const char *>(arg5);
    const auto window = std::make_shared<ToplevelWindow>(
        w, h, screen_config.pixel_format, title);

    __asm__("cli");
    const auto layer_id = layer_manager->NewLayer()
                              .SetWindow(window)
                              .SetDraggable(true)
                              .Move({x, y})
                              .ID();
    active_layer->Activate(layer_id);
    __asm__("sti");

    return {layer_id, 0};
}

SYSCALL(WinWriteString) {
    const unsigned int layer_id = arg1;
    const int x = arg2, y = arg3;
    const uint32_t color = arg4;
    const auto s = reinterpret_cast<const char *>(arg5);

    __asm__("cli");
    auto layer = layer_manager->FindLayer(layer_id);
    __asm__("sti");

    if (layer == nullptr) {
        return {0, EBADF};
    }

    WriteString(*layer->GetWindow()->Drawer(), {x, y}, s, ToColor(color));

    __asm__("cli");
    layer_manager->Draw(layer_id);
    __asm__("sti");

    return {0, 0};
}

#undef SYSCALL
}  // namespace syscall

using SyscallFuncType = syscall::Result(uint64_t, uint64_t, uint64_t, uint64_t,
                                        uint64_t, uint64_t);

extern "C" std::array<SyscallFuncType *, 5> syscall_table{
    syscall::LogString,  syscall::PutString,      syscall::Exit,
    syscall::OpenWindow, syscall::WinWriteString,
};

void InitializeSysCall() {
    // syscall命令を有効化
    WriteMSR(kIA32_EFER, 0x0501u);

    // syscallが実行された時に呼び出される関数を登録
    WriteMSR(kIA32_LSTAR, reinterpret_cast<uint64_t>(SyscallEntry));

    // syscall命令を実行するためのCS,SSを設定
    WriteMSR(kIA32_STAR, static_cast<uint64_t>(8) << 32 |
                             static_cast<uint64_t>(16 | 3) << 48);

    // syscall命令を実行するためのRFLAGSを設定
    WriteMSR(kIA32_FMASK, 0);
}