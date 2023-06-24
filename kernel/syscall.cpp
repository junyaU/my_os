#include "syscall.hpp"

#include <array>
#include <cstdint>

#include "asmfunc.h"
#include "console.hpp"
#include "msr.hpp"

namespace syscall {
#define SYSCALL(name)                                                        \
    int64_t name(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, \
                 uint64_t arg5, uint64_t arg6)

SYSCALL(LogString) {
    const char *s = reinterpret_cast<const char *>(arg1);
    if (strlen(s) > 1024) {
        return -1;
    }

    printk("%s", s);

    return 0;
}

#undef SYSCALL
}  // namespace syscall

using SyscallFuncType = int64_t(uint64_t, uint64_t, uint64_t, uint64_t,
                                uint64_t, uint64_t);

extern "C" std::array<SyscallFuncType *, 1> syscall_table{
    syscall::LogString,
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