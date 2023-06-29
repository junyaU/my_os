#include <cstddef>
#include <cstdint>

extern "C" {
struct SyscallResult {
    uint64_t value;
    int error;
};

SyscallResult SyscallLogString(const char* message);
SyscallResult SyscallPutString(int fd, const char* s, size_t len);
void SyscallExit(int exit_code);
SyscallResult SyscallOpenWindow(int w, int h, int x, int y, const char* title);
}