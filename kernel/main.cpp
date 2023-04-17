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
#include "task.hpp"
#include "timer.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"
#include "x86_descriptor.hpp"

void operator delete(void *obj) noexcept {}

std::shared_ptr<ToplevelWindow> main_window;
unsigned int main_window_layer_id;
void InitializeMainWindow() {
    main_window = std::make_shared<ToplevelWindow>(
        160, 52, screen_config.pixel_format, "Uchizono desu");

    main_window_layer_id = layer_manager->NewLayer()
                               .SetWindow(main_window)
                               .Move({300, 100})
                               .SetDraggable(true)
                               .ID();

    layer_manager->UpDown(main_window_layer_id,
                          std::numeric_limits<int>::max());
}

std::shared_ptr<ToplevelWindow> text_window;
unsigned int text_window_layer_id;
void InitializeTextWindow() {
    const int width = 160;
    const int height = 52;
    text_window = std::make_shared<ToplevelWindow>(
        width, height, screen_config.pixel_format, "Text Box Test");
    DrawTextbox(*text_window->InnerDrawer(), {0, 0}, text_window->InnerSize());

    text_window_layer_id = layer_manager->NewLayer()
                               .SetWindow(text_window)
                               .SetDraggable(true)
                               .Move({350, 200})
                               .ID();

    layer_manager->UpDown(text_window_layer_id,
                          std::numeric_limits<int>::max());
}

int text_window_index;
const int text_window_vertical_margin = 8;

void DrawTextCursor(bool visible) {
    const auto color = visible ? ToColor(0) : ToColor(0xffffff);
    const auto position = Vector2D<int>{4 + 8 * text_window_index, 5};
    FillRectangle(*text_window->InnerDrawer(), position,
                  {kFontHorizonPixels - 1, kFontVerticalPixels - 1}, color);
}

void InputTextWindow(char c) {
    if (c == 0) {
        return;
    }

    auto pos = []() { return Vector2D<int>{4 + 8 * text_window_index, 6}; };

    const int max_chars = (text_window->InnerSize().x - 8) / 8 - 1;

    if (c == '\b' && text_window_index > 0) {
        DrawTextCursor(false);
        --text_window_index;
        FillRectangle(*text_window->InnerDrawer(), pos(),
                      {kFontHorizonPixels, kFontVerticalPixels},
                      ToColor(0xffffff));
        DrawTextCursor(true);
    } else if (c >= ' ' && text_window_index < max_chars) {
        DrawTextCursor(false);
        WriteAscii(*text_window->InnerDrawer(), pos(), c, ToColor(0));
        ++text_window_index;
        DrawTextCursor(true);
    }

    layer_manager->Draw(text_window_layer_id);
}

std::shared_ptr<ToplevelWindow> task_b_window;
unsigned int task_b_window_layer_id;
void InitializeTaskBWindow() {
    task_b_window = std::make_shared<ToplevelWindow>(
        160, 52, screen_config.pixel_format, "TaskB Window");

    task_b_window_layer_id = layer_manager->NewLayer()
                                 .SetWindow(task_b_window)
                                 .SetDraggable(true)
                                 .Move({100, 100})
                                 .ID();

    layer_manager->UpDown(task_b_window_layer_id,
                          std::numeric_limits<int>::max());
}

void TaskB(uint64_t task_id, int64_t data) {
    printk("TaskB: task_id=%d, data=%d\n", task_id, data);
    char str[128];
    int count = 0;

    __asm__("cli");
    Task &task = task_manager->CurrentTask();
    __asm__("sti");

    while (true) {
        ++count;
        sprintf(str, "%10d", count);
        FillRectangle(*task_b_window->InnerDrawer(), {20, 4}, {8 * 10, 16},
                      {0xc6, 0xc6, 0xc6});
        WriteString(*task_b_window->InnerDrawer(), {20, 4}, str, {0, 0, 0});

        Message msg{Message::kLayer, task_id};
        msg.arg.layer.layer_id = task_b_window_layer_id;
        msg.arg.layer.op = LayerOperation::Draw;

        __asm__("cli");
        task_manager->SendMessage(1, msg);
        __asm__("sti");

        while (true) {
            __asm__("cli");
            auto msg = task.ReceiveMessage();
            if (!msg) {
                task.Sleep();
                __asm__("sti");
                continue;
            }

            if (msg->type == Message::kLayerFinish) {
                break;
            }
        }
    }
}

std::deque<Message> *main_queue;

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(
    const FrameBufferConfig &frame_buffer_config_ref,
    const MemoryMap &memory_map_ref, const acpi::RSDP &acpi_table) {
    FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    InitializeDrawings(frame_buffer_config);

    InitializeConsole();

    SetLogLevel(kWarn);

    InitializeSegmentation();

    InitializePaging();

    InitializeMemoryManager(memory_map);

    InitializeInterrupt();

    InitializePCI();

    InitializeLayer();

    InitializeMainWindow();

    InitializeTextWindow();

    InitializeTaskBWindow();

    layer_manager->Draw({{0, 0}, ScreenSize()});

    acpi::Initialize(acpi_table);
    InitializeLAPICTimer();

    const int kTextboxCursorTimer = 1;
    const int kTimer05Sec = static_cast<int>(kTimerFreq * 0.5);
    __asm__("cli");
    timer_manager->AddTimer(Timer{kTimer05Sec, kTextboxCursorTimer});
    __asm__("sti");
    bool textbox_cursor_visible = false;

    InitializeTask();
    Task &main_task = task_manager->CurrentTask();
    const uint64_t task_b_id =
        task_manager->NewTask().InitContext(TaskB, 45).Wakeup().ID();

    usb::xhci::Initialize();
    InitializeKeyboard();
    InitializeMouse();

    active_layer->Activate(task_b_window_layer_id);

    char str[128];

    while (true) {
        __asm__("cli");
        const auto tick = timer_manager->CurrentTick();
        __asm__("sti");

        sprintf(str, "%010lu", tick);
        FillRectangle(*main_window->InnerDrawer(), {20, 4}, {8 * 10, 16},
                      {0xc6, 0xc6, 0xc6});
        WriteString(*main_window->InnerDrawer(), {20, 4}, str, {0, 0, 0});
        layer_manager->Draw(main_window_layer_id);

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
                if (msg->arg.timer.value == kTextboxCursorTimer) {
                    __asm__("cli");
                    timer_manager->AddTimer(
                        Timer{msg->arg.timer.timeout + kTimer05Sec,
                              kTextboxCursorTimer});
                    __asm__("sti");
                    textbox_cursor_visible = !textbox_cursor_visible;
                    DrawTextCursor(textbox_cursor_visible);
                    layer_manager->Draw(text_window_layer_id);
                }
                break;
            case Message::kKeyPush:
                if (auto act = active_layer->GetActiveLayer();
                    act == text_window_layer_id) {
                    InputTextWindow(msg->arg.keyboard.ascii);
                } else if (act == task_b_window_layer_id) {
                    if (msg->arg.keyboard.ascii == 's') {
                        printk("sleep TaskB: %s\n",
                               task_manager->Sleep(task_b_id).Name());
                    } else if (msg->arg.keyboard.ascii == 'w') {
                        printk("wakeup TaskB: %s\n",
                               task_manager->Wakeup(task_b_id).Name());
                    }
                } else {
                    printk("key push not handled: keycode %02x, ascii %02x\n",
                           msg->arg.keyboard.keycode, msg->arg.keyboard.ascii);
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
