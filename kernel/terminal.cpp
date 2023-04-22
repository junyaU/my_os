#include "terminal.hpp"

#include "console.hpp"
#include "font.hpp"
#include "layer.hpp"

Terminal::Terminal() {
    window_ = std::make_shared<ToplevelWindow>(
        kColumns * kFontHorizonPixels + 8 + ToplevelWindow::kMarginX,
        kRows * kFontVerticalPixels + 8 + ToplevelWindow::kMarginY,
        screen_config.pixel_format, "JunyaTerm");

    DrawTerminal(*window_->InnerDrawer(), {0, 0}, window_->InnerSize());

    layer_id_ =
        layer_manager->NewLayer().SetWindow(window_).SetDraggable(true).ID();
}

Rectangle<int> Terminal::BlinkCursor() {
    cursor_visible_ = !cursor_visible_;
    DrawCursor(cursor_visible_);

    return {CalcCursorPos(), {7, 15}};
}

void Terminal::DrawCursor(bool visible) {
    const auto color = visible ? ToColor(0xffffff) : ToColor(0);
    FillRectangle(*window_->InnerDrawer(), CalcCursorPos(), {7, 15}, color);
}

Vector2D<int> Terminal::CalcCursorPos() const {
    return ToplevelWindow::kTopLeftMargin +
           Vector2D<int>{4 + kFontHorizonPixels * cursor_pos_.x,
                         4 + kFontVerticalPixels * cursor_pos_.y};
}

Rectangle<int> Terminal::InputKey(uint8_t modifier, uint8_t keycode,
                                  char ascii) {
    DrawCursor(false);

    Rectangle<int> draw_area{CalcCursorPos(),
                             {kFontHorizonPixels * 2, kFontVerticalPixels}};

    if (ascii == '\n') {
        linebuf_[linebuf_index_] = 0;
        linebuf_index_ = 0;
        cursor_pos_.x = 0;

        if (cursor_pos_.y < kRows - 1) {
            ++cursor_pos_.y;
        } else {
            Scroll1();
        }

        draw_area.pos = ToplevelWindow::kTopLeftMargin;
        draw_area.size = window_->InnerSize();
    } else if (ascii == '\b' && cursor_pos_.x > 0) {
        --cursor_pos_.x;
        FillRectangle(*window_->Drawer(), CalcCursorPos(),
                      {kFontHorizonPixels, kFontVerticalPixels}, {0, 0, 0});

        draw_area.pos = CalcCursorPos();

        if (linebuf_index_ > 0) {
            --linebuf_index_;
        }
    } else if (ascii != 0 && cursor_pos_.x < kColumns - 1 &&
               linebuf_index_ < kLineMax - 1) {
        linebuf_[linebuf_index_] = ascii;
        ++linebuf_index_;

        WriteAscii(*window_->Drawer(), CalcCursorPos(), ascii, {255, 255, 255});
        ++cursor_pos_.x;
    }

    DrawCursor(true);

    return draw_area;
}

void Terminal::Scroll1() {
    Rectangle<int> move_src{
        ToplevelWindow::kTopLeftMargin +
            Vector2D<int>{4, 4 + kFontVerticalPixels},
        {kColumns * kFontHorizonPixels, (kRows - 1) * kFontVerticalPixels}};

    window_->Move(ToplevelWindow::kTopLeftMargin + Vector2D<int>{4, 4},
                  move_src);

    FillRectangle(
        *window_->InnerDrawer(), {4, 4 + kFontVerticalPixels * cursor_pos_.y},
        {kFontHorizonPixels * kColumns, kFontVerticalPixels}, {0, 0, 0});
}

void TaskTerminal(uint64_t task_id, int64_t data) {
    __asm__("cli");
    Task& task = task_manager->CurrentTask();
    Terminal* terminal = new Terminal;

    layer_manager->Move(terminal->LayerID(), {100, 200});
    active_layer->Activate(terminal->LayerID());

    layer_task_map->insert(std::make_pair(terminal->LayerID(), task_id));
    __asm__("sti");

    while (true) {
        __asm__("cli");
        auto msg = task.ReceiveMessage();
        if (!msg) {
            task.Sleep();
            __asm__("sti");
            continue;
        }

        switch (msg->type) {
            case Message::kTimerTimeout: {
                const auto area = terminal->BlinkCursor();
                Message msg = MakeLayerMessage(task_id, terminal->LayerID(),
                                               LayerOperation::DrawArea, area);

                __asm__("cli");
                task_manager->SendMessage(1, msg);
                __asm__("sti");
            } break;
            case Message::kKeyPush: {
                const auto area = terminal->InputKey(msg->arg.keyboard.modifier,
                                                     msg->arg.keyboard.keycode,
                                                     msg->arg.keyboard.ascii);

                Message msg = MakeLayerMessage(task_id, terminal->LayerID(),
                                               LayerOperation::DrawArea, area);

                __asm__("cli");
                task_manager->SendMessage(1, msg);
                __asm__("sti");
            } break;
            default:
                break;
        }
    }
}
