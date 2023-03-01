#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <numeric>
#include <vector>

#include "asmfunc.h"
#include "console.hpp"
#include "drawing.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "interrupt.hpp"
#include "layer.hpp"
#include "memory_manager.hpp"
#include "memory_map.hpp"
#include "message.hpp"
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "segment.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"
#include "x86_descriptor.hpp"

void operator delete(void *obj) noexcept {}

int printk(const char format[], ...) {
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    console->PutString(s);
    return result;
}

unsigned int mouse_layer_id;
Vector2D<int> screen_size;
Vector2D<int> mouse_position;

void MouseObserver(uint8_t buttons, int8_t displacement_x,
                   int8_t displacement_y) {
    static unsigned int mouse_drag_layer_id = 0;
    static uint8_t previous_buttons = 0;

    auto oldpos = mouse_position;

    auto newpos =
        mouse_position + Vector2D<int>{displacement_x, displacement_y};
    newpos = ElementMin(
        newpos,
        screen_size + Vector2D<int>{-kMouseCursorWidth, -kMouseCursorHeight});
    mouse_position = ElementMax(newpos, {0, 0});

    const auto posdiff = mouse_position - oldpos;

    layer_manager->Move(mouse_layer_id, mouse_position);

    const bool previous_left_pressed = (previous_buttons & 0x01);
    const bool left_pressed = (buttons & 0x01);
    if (!previous_left_pressed && left_pressed) {
        auto layer =
            layer_manager->FindLayerByPoisition(mouse_position, mouse_layer_id);
        if (layer) {
            mouse_drag_layer_id = layer->ID();
        }
    } else if (previous_left_pressed && left_pressed) {
        if (mouse_drag_layer_id > 0) {
            layer_manager->MoveRelative(mouse_drag_layer_id, posdiff);
        }
    } else if (!previous_left_pressed && !left_pressed) {
        mouse_drag_layer_id = 0;
    }

    previous_buttons = buttons;
}

std::deque<Message> *main_queue;

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(
    const FrameBufferConfig &frame_buffer_config_ref,
    const MemoryMap &memory_map_ref) {
    FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    InitializeDrawings(frame_buffer_config);

    InitializeConsole();

    printk("hello");

    InitializeSegmentation();

    InitializePaging();

    InitializeMemoryManager(memory_map);

    ::main_queue = new std::deque<Message>(32);
    InitializeInterrupt(main_queue);

    InitializePCI();
    usb::xhci::Initialize();

    screen_size.x = frame_buffer_config.horizontal_resolution;
    screen_size.y = frame_buffer_config.vertical_resolution;

    auto mouse_window =
        std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight,
                                 frame_buffer_config.pixel_format);
    mouse_window->SetTransparentColor(kMouseTransparentColor);
    DrawMouseCursor(mouse_window->Drawer(), {0, 0});
    mouse_position = {200, 200};

    auto main_window =
        std::make_shared<Window>(160, 52, frame_buffer_config.pixel_format);
    DrawWindow(*main_window->Drawer(), "UCH Window");

    auto main_window_layer_id = layer_manager->NewLayer()
                                    .SetWindow(main_window)
                                    .SetDraggable(true)
                                    .Move({300, 100})
                                    .ID();

    layer_manager->UpDown(main_window_layer_id, 2);
    layer_manager->UpDown(mouse_layer_id, 3);
    layer_manager->Draw({{0, 0}, screen_size});

    char str[128];
    unsigned int count = 0;

    while (true) {
        ++count;
        sprintf(str, "%010u", count);
        FillRectangle(*main_window->Drawer(), {24, 28}, {8 * 10, 16},
                      {0xc6, 0xc6, 0xc6});
        WriteString(*main_window->Drawer(), {24, 28}, str, {0, 0, 0});
        layer_manager->Draw(main_window_layer_id);

        __asm__("cli");
        if (main_queue->size() == 0) {
            __asm__("sti");
            continue;
        }

        Message msg = main_queue->front();
        main_queue->pop_front();
        __asm__("sti");

        switch (msg.type) {
            case Message::kInterruptXHCI:
                usb::xhci::ProcessEvents();
                break;
            default:
                printk("Unknown message type: %d\n", msg.type);
        }
    }

    while (1) __asm__("hlt");
}

extern "C" void __cxa_pure_virtual() {
    while (1) __asm__("hlt");
}
