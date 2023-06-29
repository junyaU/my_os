#include "../syscall.h"

extern "C" void main() {
    auto [layer_id, err] = SyscallOpenWindow(200, 100, 10, 10, " hello");
    if (err) {
        SyscallExit(err);
    }

    SyscallWinWriteString(layer_id, 7, 24, 0xc00000, "hello world");
    SyscallWinWriteString(layer_id, 24, 40, 0x00c000, "hikakin");
    SyscallWinWriteString(layer_id, 40, 56, 0x0000c0, "seikin");

    SyscallExit(0);
}