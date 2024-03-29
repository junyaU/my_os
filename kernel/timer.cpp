#include "timer.hpp"

#include "acpi.hpp"
#include "interrupt.hpp"
#include "task.hpp"

namespace {
const uint32_t kCountMax = 0xffffffffu;

// 割り込み発生方法の設定
volatile uint32_t& lvt_timer = *reinterpret_cast<uint32_t*>(0xfee00320);
// カウンタの初期値
volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(0xfee00380);
// 現在のカウンタの値
volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(0xfee00390);
// カウント速度
volatile uint32_t& divide_config = *reinterpret_cast<uint32_t*>(0xfee003e0);
}  // namespace

void InitializeLAPICTimer() {
    timer_manager = new TimerManager;

    divide_config = 0b1011;
    lvt_timer = (0b001 << 16);

    StartLAPICTimer();

    acpi::Wait(100);
    const auto elapsed = LAPICTimerElapsed();

    StopLAPICTimer();

    lapic_timer_freq = static_cast<unsigned long>(elapsed) * 10;

    lvt_timer = (0b010 << 16) | InterruptVector::kLAPICTimer;
    initial_count = lapic_timer_freq / kTimerFreq;
}

void StartLAPICTimer() { initial_count = kCountMax; }

uint32_t LAPICTimerElapsed() { return kCountMax - current_count; }

void StopLAPICTimer() { initial_count = 0; }

Timer::Timer(unsigned long timeout, int value, uint64_t task_id)
    : timeout_{timeout}, value_{value}, task_id_{task_id} {}

TimerManager::TimerManager() {
    timers_.push(Timer{std::numeric_limits<unsigned long>::max(), -1, 1});
}

void TimerManager::AddTimer(const Timer& timer) { timers_.push(timer); }

TimerManager* timer_manager;
unsigned long lapic_timer_freq;

bool TimerManager::Tick() {
    ++tick_;

    bool task_timer_timeout = false;
    while (true) {
        const auto& timer = timers_.top();
        if (timer.Timeout() > tick_) {
            break;
        }

        if (timer.Value() == kTaskTimerValue) {
            task_timer_timeout = true;
            timers_.pop();
            timers_.push(Timer{tick_ + kTaskTimerPeriod, kTaskTimerValue, 1});
            continue;
        }

        Message message{Message::kTimerTimeout};
        message.arg.timer.timeout = timer.Timeout();
        message.arg.timer.value = timer.Value();
        task_manager->SendMessage(timer.TaskID(), message);

        timers_.pop();
    }

    return task_timer_timeout;
}

extern "C" void LAPICTimerOnInterrupt(const TaskContext& ctx_stack) {
    const bool task_timer_timeout = timer_manager->Tick();
    NotifyEndOfInterrupt();

    if (task_timer_timeout) {
        task_manager->SwitchTask(ctx_stack);
    }
}
