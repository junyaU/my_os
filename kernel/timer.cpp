#include "timer.hpp"

#include "interrupt.hpp"

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
    lvt_timer = (0b010 << 16) | InterruptVector::kLAPICTimer;
    initial_count = 0x1000000u;
}

void StartLAPICTimer() { initial_count = kCountMax; }

uint32_t LAPICTimerElapsed() { return kCountMax - current_count; }

void StopLAPICTimer() { initial_count = 0; }

TimerManager* timer_manager;

void TimerManager::Tick() { ++tick_; }

void LAPICTimerOnInterrupt() { timer_manager->Tick(); }
