#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "queue.hpp"
#include "segment.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"
#include "x86_descriptor.hpp"

void operator delete(void *obj) noexcept {}

char screen_drawer_buffer[sizeof(RGB8BitScreenDrawer)];
ScreenDrawer *screen_drawer;

char console_buf[sizeof(Console)];
Console *console;

char memory_manager_buffer[sizeof(BitmapMemoryManager)];
BitmapMemoryManager *memory_manager;

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

usb::xhci::Controller *xhc;

struct Message {
    enum Type {
        kInterruptXHCI,
    } type;
};

ArrayQueue<Message> *main_queue;

__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame) {
    main_queue->ENQ(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
}

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(
    const FrameBufferConfig &frame_buffer_config_ref,
    const MemoryMap &memory_map_ref) {
    FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    switch (frame_buffer_config.pixel_format) {
        case kPixelRGBResv8BitPerColor:
            screen_drawer = new (screen_drawer_buffer)
                RGB8BitScreenDrawer{frame_buffer_config};
            break;
        case kPixelBGRResv8BitPerColor:
            screen_drawer = new (screen_drawer_buffer)
                BGR8BitScreenDrawer{frame_buffer_config};
            break;
    }

    DrawDesktop(*screen_drawer);

    console = new (console_buf) Console{kDesktopFGColor, kDesktopBGColor};
    console->SetDrawer(screen_drawer);

    SetupSegments();

    const uint16_t kernel_cs = 1 << 3;
    const uint16_t kernel_ss = 2 << 3;
    SetDSAll(0);
    SetCSSS(kernel_cs, kernel_ss);

    // メモリのページング設定
    SetupIdentityPageTable();

    ::memory_manager = new (memory_manager_buffer) BitmapMemoryManager;
    const auto memory_map_base_address =
        reinterpret_cast<uintptr_t>(memory_map.buffer);

    uintptr_t available_end = 0;
    for (uintptr_t iter = memory_map_base_address;
         iter < memory_map_base_address + memory_map.map_size;
         iter += memory_map.descriptor_size) {
        auto descriptor = reinterpret_cast<const MemoryDescriptor *>(iter);
        if (available_end < descriptor->physical_start) {
            memory_manager->MarkAllocated(
                FrameID{available_end / kBytesPerFrame},
                (descriptor->physical_start - available_end) / kBytesPerFrame);
        }

        const auto physical_end_address =
            descriptor->physical_start +
            descriptor->number_of_pages * kUEFIPageSize;

        if (IsAvailable(static_cast<MemoryType>(descriptor->type))) {
            available_end = physical_end_address;
        } else {
            memory_manager->MarkAllocated(
                FrameID{descriptor->physical_start / kBytesPerFrame},
                descriptor->number_of_pages * kUEFIPageSize / kBytesPerFrame);
        }
    }

    memory_manager->SetMemoryRange(FrameID{1},
                                   FrameID{available_end / kBytesPerFrame});

    if (auto err = InitializeHeap(*memory_manager)) {
        printk("failed to allocate pages: %s at %s:%d\n", err.Name(),
               err.File(), err.Line());
        exit(1);
    }

    std::array<Message, 32> main_queue_data;
    ArrayQueue<Message> main_queue{main_queue_data};
    ::main_queue = &main_queue;

    auto err = pci::ScanAllDevice();
    printk("ScanAllBus: %s\n", err.Name());

    for (int i = 0; i < pci::num_device; ++i) {
        const auto &device = pci::devices[i];
        auto vender_id =
            pci::ReadVendorId(device.bus, device.device, device.function);
        auto class_code =
            pci::ReadClassCode(device.bus, device.device, device.function);

        printk("%d.%d.%d: vend %04x, class %08x, head %02x\n", device.bus,
               device.device, device.function, vender_id, class_code,
               device.header_type);
    }

    pci::Device *xhc_device = nullptr;
    for (int i = 0; i < pci::num_device; ++i) {
        if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
            xhc_device = &pci::devices[i];

            if (0x8086 == pci::ReadVendorId(*xhc_device)) {
                break;
            }
        }
    }

    if (xhc_device) {
        printk("xHC has been found: %d.%d.%d\n", xhc_device->bus,
               xhc_device->device, xhc_device->function);
    }

    const uint16_t cs = GetCS();
    SetIDTEntry(idt[InterruptVector::kXHCI],
                MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);

    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

    const uint8_t bsp_local_apic_id =
        *reinterpret_cast<const uint32_t *>(0xfee00020) >> 24;

    pci::ConfigureMSIFixedDestination(
        *xhc_device, bsp_local_apic_id, pci::MSITriggerMode::kLevel,
        pci::MSIDeliveryMode::kFixed, InterruptVector::kXHCI, 0);

    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_device, 0);
    printk("ReadBar: %s\n", xhc_bar.error.Name());
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    printk("xHC mmio_base = %08lx\n", xhc_mmio_base);

    usb::xhci::Controller xhc{xhc_mmio_base};

    {
        auto err = xhc.Initialize();

        printk("xhc.Initialize: %s\n", err.Name());
    }

    printk("xHC starting\n");
    xhc.Run();

    ::xhc = &xhc;

    usb::HIDMouseDriver::default_observer = MouseObserver;

    for (int i = 1; i <= xhc.MaxPorts(); ++i) {
        auto port = xhc.PortAt(i);

        if (!port.IsConnected()) {
            continue;
        }

        if (auto err = usb::xhci::ConfigurePort(xhc, port)) {
            printk("failed to configure port: %s at %s:%d\n", err.Name(),
                   err.File(), err.Line());
        }
    }

    screen_size.x = frame_buffer_config.horizontal_resolution;
    screen_size.y = frame_buffer_config.vertical_resolution;

    auto bgWindow = std::make_shared<Window>(screen_size.x, screen_size.y,
                                             frame_buffer_config.pixel_format);
    auto bgDrawer = bgWindow->Drawer();

    DrawDesktop(*bgDrawer);

    auto mouse_window =
        std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight,
                                 frame_buffer_config.pixel_format);
    mouse_window->SetTransparentColor(kMouseTransparentColor);
    DrawMouseCursor(mouse_window->Drawer(), {0, 0});
    mouse_position = {200, 200};

    auto main_window =
        std::make_shared<Window>(160, 52, frame_buffer_config.pixel_format);
    DrawWindow(*main_window->Drawer(), "UCH Window");

    auto console_window =
        std::make_shared<Window>(Console::kColumns * 8, Console::kRows * 16,
                                 frame_buffer_config.pixel_format);
    console->SetWindow(console_window);

    FrameBuffer screen;
    if (auto err = screen.Initialize(frame_buffer_config)) {
        printk("failed to initialize frame buffer: %s at %s:%d\n", err.Name(),
               err.File(), err.Line());
    }

    layer_manager = new LayerManager;
    layer_manager->SetDrawer(&screen);

    auto bglayer_id =
        layer_manager->NewLayer().SetWindow(bgWindow).Move({0, 0}).ID();
    mouse_layer_id = layer_manager->NewLayer()
                         .SetWindow(mouse_window)
                         .Move(mouse_position)
                         .ID();
    auto main_window_layer_id = layer_manager->NewLayer()
                                    .SetWindow(main_window)
                                    .SetDraggable(true)
                                    .Move({300, 100})
                                    .ID();

    console->SetLayerID(
        layer_manager->NewLayer().SetWindow(console_window).Move({0, 0}).ID());

    layer_manager->UpDown(bglayer_id, 0);
    layer_manager->UpDown(console->LayerID(), 1);
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
        if (main_queue.Count() == 0) {
            __asm__("sti");
            continue;
        }

        Message msg = main_queue.Front();
        main_queue.DEQ();
        __asm__("sti");

        switch (msg.type) {
            case Message::kInterruptXHCI:
                while (xhc.PrimaryEventRing()->HasFront()) {
                    if (auto err = usb::xhci::ProcessEvent(xhc)) {
                        printk("Error while ProcessEvent: %s at %s:%d\n",
                               err.Name(), err.File(), err.Line());
                    }
                }
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
