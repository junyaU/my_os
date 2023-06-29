#include "../syscall.h"

extern "C" void main() {
    SyscallOpenWindow(200, 100, 10, 10, " hello");
    SyscallExit(0);
}