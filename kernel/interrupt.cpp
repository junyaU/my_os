#include "interrupt.hpp"

#include "asmfunc.h"
#include "drawing.hpp"
#include "font.hpp"
#include "message.hpp"
#include "segment.hpp"
#include "task.hpp"
#include "timer.hpp"

std::array<InterruptDescriptor, 256> idt;

void SetIDTEntry(InterruptDescriptor &descriptor,
                 InterruptDescriptorAttribute attr, uint64_t offset,
                 uint16_t segment_selector) {
    descriptor.attr = attr;
    descriptor.offset_low = offset & 0xffffu;
    descriptor.offset_middle = (offset >> 16) & 0xffffu;
    descriptor.offset_high = offset >> 32;
    descriptor.segment_selector = segment_selector;
}

void NotifyEndOfInterrupt() {
    volatile auto end_of_interrupt = reinterpret_cast<uint32_t *>(0xfee000b0);
    *end_of_interrupt = 0;
}

namespace {
__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame) {
    task_manager->SendMessage(1, Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
}

__attribute__((interrupt)) void IntHandlerLAPICTimer(InterruptFrame *frame) {
    LAPICTimerOnInterrupt();
}

void PrintHex(uint64_t value, int width, Vector2D<int> pos) {
    for (int i = 0; i < width; i++) {
        int x = (value >> 4 * (width - i - 1)) & 0xfu;
        if (x >= 10) {
            x += 'a' - 10;
        } else {
            x += '0';
        }

        WriteAscii(*screen_drawer,
                   pos + Vector2D<int>{kFontHorizonPixels * i, 0}, x,
                   {255, 255, 255});
    }
}

void PrintFrame(InterruptFrame *frame, const char *exp_name) {
    auto output_pos = 500;
    PixelColor font_color = {255, 255, 255};

    WriteString(*screen_drawer, {output_pos, kFontVerticalPixels * 0}, exp_name,
                font_color);
    WriteString(*screen_drawer, {output_pos, kFontVerticalPixels * 1}, "CS:RIP",
                font_color);
    PrintHex(frame->cs, 4,
             {output_pos + kFontHorizonPixels * 7, kFontVerticalPixels * 1});
    PrintHex(frame->rip, 16,
             {output_pos + kFontHorizonPixels * 12, kFontVerticalPixels * 1});
    WriteString(*screen_drawer, {output_pos, kFontVerticalPixels * 2}, "RFLAGS",
                font_color);
    PrintHex(frame->rflags, 16,
             {output_pos + kFontHorizonPixels * 7, kFontVerticalPixels * 2});
    WriteString(*screen_drawer, {output_pos, kFontVerticalPixels * 3}, "SS:RSP",
                font_color);
    PrintHex(frame->ss, 4,
             {output_pos + kFontHorizonPixels * 7, kFontVerticalPixels * 3});
    PrintHex(frame->rsp, 16,
             {output_pos + kFontHorizonPixels * 12, kFontVerticalPixels * 3});
}

#define FaultHandlerWithError(fault_name)                                  \
    __attribute__((interrupt)) void IntHandler##fault_name(                \
        InterruptFrame *frame, uint64_t error_code) {                      \
        PrintFrame(frame, "#" #fault_name);                                \
        WriteString(*screen_drawer, {500, kFontVerticalPixels * 4}, "ERR", \
                    {255, 255, 255});                                      \
        PrintHex(error_code, 16,                                           \
                 {500 + kFontHorizonPixels * 4, kFontVerticalPixels * 4}); \
        while (1) __asm__("hlt");                                          \
    }

#define FaultHandlerNoError(fault_name)                     \
    __attribute__((interrupt)) void IntHandler##fault_name( \
        InterruptFrame *frame) {                            \
        PrintFrame(frame, "#" #fault_name);                 \
        while (1) __asm__("hlt");                           \
    }

FaultHandlerNoError(DE) FaultHandlerNoError(DB) FaultHandlerNoError(BP)
    FaultHandlerNoError(OF) FaultHandlerNoError(BR) FaultHandlerNoError(UD)
        FaultHandlerNoError(NM) FaultHandlerWithError(DF)
            FaultHandlerWithError(TS) FaultHandlerWithError(NP)
                FaultHandlerWithError(SS) FaultHandlerWithError(GP)
                    FaultHandlerWithError(PF) FaultHandlerNoError(MF)
                        FaultHandlerWithError(AC) FaultHandlerNoError(MC)
                            FaultHandlerNoError(XM) FaultHandlerNoError(VE)
}  // namespace

void InitializeInterrupt() {
    auto set_idt_entry = [](int irq, auto handler) {
        SetIDTEntry(idt[irq], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                    reinterpret_cast<uint64_t>(handler), kKernelCS);
    };

    set_idt_entry(InterruptVector::kXHCI, IntHandlerXHCI);
    set_idt_entry(InterruptVector::kLAPICTimer, IntHandlerLAPICTimer);

    // Exception Handler
    set_idt_entry(0, IntHandlerDE);
    set_idt_entry(1, IntHandlerDB);
    set_idt_entry(3, IntHandlerBP);
    set_idt_entry(4, IntHandlerOF);
    set_idt_entry(5, IntHandlerBR);
    set_idt_entry(6, IntHandlerUD);
    set_idt_entry(7, IntHandlerNM);
    set_idt_entry(8, IntHandlerDF);
    set_idt_entry(10, IntHandlerTS);
    set_idt_entry(11, IntHandlerNP);
    set_idt_entry(12, IntHandlerSS);
    set_idt_entry(13, IntHandlerGP);
    set_idt_entry(14, IntHandlerPF);
    set_idt_entry(16, IntHandlerMF);
    set_idt_entry(17, IntHandlerAC);
    set_idt_entry(18, IntHandlerMC);
    set_idt_entry(19, IntHandlerXM);
    set_idt_entry(20, IntHandlerVE);

    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
}