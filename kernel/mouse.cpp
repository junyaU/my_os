#include "mouse.hpp"

#include <limits>
#include <memory>

#include "drawing.hpp"
#include "layer.hpp"
#include "task.hpp"
#include "usb/classdriver/mouse.hpp"

namespace {
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ", "@@             ", "@.@            ", "@..@           ",
    "@...@          ", "@....@         ", "@.....@        ", "@......@       ",
    "@.......@      ", "@........@     ", "@.........@    ", "@..........@   ",
    "@...........@  ", "@............@ ", "@......@@@@@@@@", "@......@       ",
    "@....@@.@      ", "@...@ @.@      ", "@..@   @.@     ", "@.@    @.@     ",
    "@@      @.@    ", "@       @.@    ", "         @.@   ", "         @@@   ",
};
}  // namespace

void SendMouseMessage(Vector2D<int> new_pos, Vector2D<int> posdiff,
                      uint8_t buttons) {
    const auto act_id = active_layer->GetActiveLayer();
    if (!act_id) {
        return;
    }

    const auto layer = layer_manager->FindLayer(act_id);

    const auto task_it = layer_task_map->find(act_id);
    if (task_it == layer_task_map->end()) {
        return;
    }

    if (posdiff.x != 0 || posdiff.y != 0) {
        const auto relative_pos = new_pos - layer->GetPosition();
        Message msg{Message::kMouseMove};
        msg.arg.mouse_move.x = relative_pos.x;
        msg.arg.mouse_move.y = relative_pos.y;
        msg.arg.mouse_move.dx = posdiff.x;
        msg.arg.mouse_move.dy = posdiff.y;
        msg.arg.mouse_move.buttons = buttons;
        task_manager->SendMessage(task_it->second, msg);
    }
}

void DrawMouseCursor(ScreenDrawer *drawer, Vector2D<int> pos) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] == '@') {
                drawer->Draw(pos, {0, 0, 0});
            } else if (mouse_cursor_shape[dy][dx] == '.') {
                drawer->Draw(pos + Vector2D<int>{dx, dy}, {255, 255, 255});
            } else {
                drawer->Draw(pos + Vector2D<int>{dx, dy},
                             kMouseTransparentColor);
            }
        }
    }
}

Mouse::Mouse(unsigned int layer_id) : layer_id_{layer_id} {}

void Mouse::SetPosition(Vector2D<int> position) {
    position_ = position;
    layer_manager->Move(layer_id_, position);
}

void Mouse::OnInterrupt(uint8_t buttons, int8_t displacement_x,
                        int8_t displacement_y) {
    const auto old_pos = position_;
    auto new_pos = position_ + Vector2D<int>{displacement_x, displacement_y};
    new_pos = ElementMin(new_pos, old_pos - Vector2D<int>{-kMouseCursorWidth,
                                                          -kMouseCursorHeight});
    position_ = ElementMax(new_pos, {0, 0});

    layer_manager->Move(layer_id_, position_);

    const auto pos_diff = position_ - old_pos;
    const bool previous_left_pressed = (previous_buttons_ & 0x01);
    const bool left_pressed = (buttons & 0x01);

    if (!previous_left_pressed && left_pressed) {
        auto layer = layer_manager->FindLayerByPoisition(position_, layer_id_);
        if (layer && layer->IsDraggable()) {
            drag_layer_id_ = layer->ID();
            active_layer->Activate(drag_layer_id_);
        } else {
            active_layer->Activate(0);
        }
    } else if (previous_left_pressed && left_pressed) {
        if (drag_layer_id_ > 0) {
            layer_manager->MoveRelative(drag_layer_id_, pos_diff);
        }
    } else if (previous_left_pressed && !left_pressed) {
        drag_layer_id_ = 0;
    }

    if (drag_layer_id_ == 0) {
        SendMouseMessage(position_, pos_diff, buttons);
    }

    previous_buttons_ = buttons;
}

void InitializeMouse() {
    auto mouse_window = std::make_shared<Window>(
        kMouseCursorWidth, kMouseCursorHeight, screen_config.pixel_format);
    mouse_window->SetTransparentColor(kMouseTransparentColor);
    DrawMouseCursor(mouse_window->Drawer(), {0, 0});

    auto mouse_layer_id =
        layer_manager->NewLayer().SetWindow(mouse_window).ID();

    auto mouse = std::make_shared<Mouse>(mouse_layer_id);
    mouse->SetPosition({200, 200});
    layer_manager->UpDown(mouse->LayerID(), std::numeric_limits<int>::max());

    usb::HIDMouseDriver::default_observer =
        [mouse](uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
            mouse->OnInterrupt(buttons, displacement_x, displacement_y);
        };

    active_layer->SetMouseLayer(mouse_layer_id);
}