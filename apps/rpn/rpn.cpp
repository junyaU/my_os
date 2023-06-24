#include <cstdint>
#include <cstdlib>
#include <cstring>

int stack_ptr;
long stack[100];

long Pop() {
    long value = stack[stack_ptr];
    --stack_ptr;
    return value;
}

void Push(long value) {
    ++stack_ptr;
    stack[stack_ptr] = value;
}

extern "C" int64_t SyscallLogString(const char *);

extern "C" int main(int argc, char **argv) {
    stack_ptr = -1;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "+") == 0) {
            long b = Pop();
            long a = Pop();
            Push(a + b);
            SyscallLogString("add");
        } else if (strcmp(argv[i], "-") == 0) {
            long b = Pop();
            long a = Pop();
            Push(a - b);
            SyscallLogString("sub");
        } else {
            long a = atol(argv[i]);
            Push(a);
            SyscallLogString("push");
        }
    }
    if (stack_ptr < 0) {
        return 0;
    }

    SyscallLogString("call rpn");

    while (true) {
    };

    // return static_cast<int>(Pop());
}
