#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <numeric>
#include <vector>

#include "acpi.hpp"
#include "asmfunc.h"
#include "console.hpp"
#include "drawing.hpp"
#include "fat.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "interrupt.hpp"
#include "keyboard.hpp"
#include "layer.hpp"
#include "logger.hpp"
#include "memory_manager.hpp"
#include "memory_map.hpp"
#include "message.hpp"
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "segment.hpp"
#include "syscall.hpp"
#include "task.hpp"
#include "terminal.hpp"
#include "timer.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"
#include "x86_descriptor.hpp"

void operator delete(void *obj) noexcept {}

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(
    const FrameBufferConfig &frame_buffer_config_ref,
    const MemoryMap &memory_map_ref, const acpi::RSDP &acpi_table,
    void *volume_image) {
    FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    InitializeDrawings(frame_buffer_config);

    InitializeConsole();

    SetLogLevel(kWarn);

    InitializeSegmentation();

    InitializePaging();

    InitializeMemoryManager(memory_map);

    InitializeTSS();

    InitializeInterrupt();

    fat::Initialize(volume_image);

    InitializeFont();

    InitializePCI();

    InitializeLayer();

    layer_manager->Draw({{0, 0}, ScreenSize()});

    acpi::Initialize(acpi_table);
    InitializeLAPICTimer();

    const int kTextboxCursorTimer = 1;
    const int kTimer05Sec = static_cast<int>(kTimerFreq * 0.5);
    timer_manager->AddTimer(Timer{kTimer05Sec, kTextboxCursorTimer, 1});

    InitializeSysCall();

    InitializeTask();
    Task &main_task = task_manager->CurrentTask();

    usb::xhci::Initialize();
    InitializeKeyboard();
    InitializeMouse();

    task_manager->NewTask().InitContext(TaskTerminal, 0).Wakeup().ID();

    while (true) {
        __asm__("cli");
        auto msg = main_task.ReceiveMessage();
        if (!msg) {
            main_task.Sleep();
            __asm__("sti");
            continue;
        }

        __asm__("sti");

        switch (msg->type) {
            case Message::kInterruptXHCI:
                usb::xhci::ProcessEvents();
                break;
            case Message::kTimerTimeout:
                break;
            case Message::kKeyPush:
                if (auto act = active_layer->GetActiveLayer();
                    msg->arg.keyboard.press &&
                    msg->arg.keyboard.keycode == 20) {
                    task_manager->NewTask()
                        .InitContext(TaskTerminal, 0)
                        .Wakeup();
                } else {
                    __asm__("cli");
                    auto task_it = layer_task_map->find(act);
                    __asm__("sti");

                    if (task_it != layer_task_map->end()) {
                        __asm__("cli");
                        task_manager->SendMessage(task_it->second, *msg);
                        __asm__("sti");
                    } else {
                        printk(
                            "key push not handled: keycode %02x, ascii %02x\n",
                            msg->arg.keyboard.keycode, msg->arg.keyboard.ascii);
                    }
                }
                break;

            case Message::kLayer:
                ProcessLayerMessage(*msg);
                __asm__("cli");
                task_manager->SendMessage(msg->source_task,
                                          Message{Message::kLayerFinish});
                __asm__("sti");
                break;
            default:
                printk("Unknown message type: %d\n", msg->type);
        }
    }
}

extern "C" void __cxa_pure_virtual() {
    while (1) __asm__("hlt");
}
