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
#include "memory_manager.hpp"
#include "memory_map.hpp"
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "segment.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"

void operator delete(void *obj) noexcept {}

const PixelColor desktop_bg_color{45, 118, 237};

char screen_drawer_buffer[sizeof(RGB8BitScreenDrawer)];
ScreenDrawer *screen_drawer;

char console_buf[sizeof(Console)];
Console *console;

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

char memory_manager_buffer[sizeof(BitmapMemoryManager)];
BitmapMemoryManager *memory_manager;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
    mouse_cursor->MoveRelative({displacement_x, displacement_y});
}

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

usb::xhci::Controller *xhc;

__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame) {
    while (xhc->PrimaryEventRing()->HasFront()) {
        if (auto err = usb::xhci::ProcessEvent(*xhc)) {
            printk("Error while ProcessEvent: %s at %s:%d\n", err.Name(),
                   err.File(), err.Line());
        }
    }

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

    const int frame_width = frame_buffer_config.horizontal_resolution;
    const int frame_height = frame_buffer_config.vertical_resolution;

    FillRectangle(*screen_drawer, {0, 0}, {frame_width, frame_height - 50},
                  desktop_bg_color);

    FillRectangle(*screen_drawer, {0, frame_height - 50}, {frame_width, 50},
                  {80, 80, 80});

    console = new (console_buf)
        Console{*screen_drawer, {255, 255, 255}, {255, 255, 255}};

    mouse_cursor = new (mouse_cursor_buf)
        MouseCursor{screen_drawer, desktop_bg_color, {300, 200}};

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
        *reinterpret_cast<const uint32_t *>(0xfee00020);

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
    __asm__("sti");

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

    while (1) __asm__("hlt");
}

extern "C" void __cxa_pure_virtual() {
    while (1) __asm__("hlt");
}
