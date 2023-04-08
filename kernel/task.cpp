#include "task.hpp"

#include "asmfunc.h"
#include "timer.hpp"

alignas(16) TaskContext task_b_ctx, task_a_ctx;

namespace {
TaskContext* current_ctx;
}

void InitializeTask() {
    current_ctx = &task_a_ctx;
    __asm__("cli");
    timer_manager->AddTimer(Timer{
        timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue});
    __asm__("sti");
}

void SwitchTask() {
    TaskContext* old_current_ctx = current_ctx;
    if (current_ctx == &task_a_ctx) {
        current_ctx = &task_b_ctx;
    } else {
        current_ctx = &task_a_ctx;
    }

    SwitchContext(current_ctx, old_current_ctx);
}