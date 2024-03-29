#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../syscall.h"

extern "C" void main() {
    auto [layer_id, err] = SyscallOpenWindow(200, 100, 10, 10, " hello");
    if (err) {
        SyscallExit(err);
    }

    SyscallWinWriteString(layer_id, 7, 24, 0xc00000, "hello world");
    SyscallWinWriteString(layer_id, 24, 40, 0x00c000, "hikakin");
    SyscallWinWriteString(layer_id, 40, 56, 0x0000c0, "seikin");

    AppEvent events[1];
    while (true) {
        auto [n, err] = SyscallReadEvent(events, 1);
        if (err) {
            printf("ReadEvent failed: %s\n", strerror(err));
            break;
        }

        if (events[0].type == AppEvent::kQuit) {
            break;
        } else {
            printf("unknown event: type = %d\n", events[0].type);
        }
    }

    SyscallCloseWindow(layer_id);
    exit(0);
}