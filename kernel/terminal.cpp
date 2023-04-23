#include "terminal.hpp"

#include "console.hpp"
#include "font.hpp"
#include "layer.hpp"
#include "pci.hpp"

Terminal::Terminal() {
    window_ = std::make_shared<ToplevelWindow>(
        kColumns * kFontHorizonPixels + 8 + ToplevelWindow::kMarginX,
        kRows * kFontVerticalPixels + 8 + ToplevelWindow::kMarginY,
        screen_config.pixel_format, "JunyaTerm");

    DrawTerminal(*window_->InnerDrawer(), {0, 0}, window_->InnerSize());

    layer_id_ =
        layer_manager->NewLayer().SetWindow(window_).SetDraggable(true).ID();

    Print(">");
    cmd_history_.resize(8);
}

Rectangle<int> Terminal::BlinkCursor() {
    cursor_visible_ = !cursor_visible_;
    DrawCursor(cursor_visible_);

    return {CalcCursorPos(), {7, 15}};
}

void Terminal::DrawCursor(bool visible) {
    const auto color = visible ? ToColor(0xffffff) : ToColor(0);
    FillRectangle(*window_->Drawer(), CalcCursorPos(), {7, 15}, color);
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

        if (linebuf_index_ > 0) {
            cmd_history_.pop_back();
            cmd_history_.push_front(linebuf_);
        }

        cmd_history_index_ = -1;
        linebuf_index_ = 0;
        cursor_pos_.x = 0;

        if (cursor_pos_.y < kRows - 1) {
            ++cursor_pos_.y;
        } else {
            Scroll1();
        }

        ExecuteLine();
        Print(">");
        draw_area.pos = ToplevelWindow::kTopLeftMargin;
        draw_area.size = window_->InnerSize();
    } else if (ascii == '\b') {
        if (cursor_pos_.x > 0) {
            --cursor_pos_.x;
            FillRectangle(*window_->Drawer(), CalcCursorPos(),
                          {kFontHorizonPixels, kFontVerticalPixels}, {0, 0, 0});

            draw_area.pos = CalcCursorPos();

            if (linebuf_index_ > 0) {
                --linebuf_index_;
            }
        };
    } else if (keycode == 0x51) {
        draw_area = HistoryUpDown(-1);
    } else if (keycode == 0x52) {
        draw_area = HistoryUpDown(1);
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

void Terminal::Print(const char* s) {
    DrawCursor(false);

    auto newline = [this]() {
        cursor_pos_.x = 0;
        if (cursor_pos_.y < kRows - 1) {
            ++cursor_pos_.y;
        } else {
            Scroll1();
        }
    };

    while (*s) {
        if (*s == '\n') {
            newline();
        } else {
            WriteAscii(*window_->Drawer(), CalcCursorPos(), *s,
                       {255, 255, 255});
            if (cursor_pos_.x == kColumns) {
                newline();
            } else {
                ++cursor_pos_.x;
            }
        }

        ++s;
    }

    DrawCursor(true);
}

void Terminal::ExecuteLine() {
    char* command = &linebuf_[0];
    char* first_arg = strchr(command, ' ');
    if (first_arg) {
        *first_arg = 0;
        ++first_arg;
    }

    if (strcmp(command, "echo") == 0) {
        if (first_arg) {
            Print(first_arg);
        }
        Print("\n");
    } else if (strcmp(command, "clear") == 0) {
        FillRectangle(
            *window_->InnerDrawer(), {4, 4},
            {kFontHorizonPixels * kColumns, kFontVerticalPixels * kRows},
            {0, 0, 0});

        cursor_pos_.y = 0;
    } else if (strcmp(command, "lspci") == 0) {
        char s[64];
        for (int i = 0; i < pci::num_device; ++i) {
            const auto& dev = pci::devices[i];
            auto vendor_id =
                pci::ReadVendorId(dev.bus, dev.device, dev.function);
            sprintf(s,
                    "%02x:%02x.%d vend=%04x head=%02x class=%02x.%02x.%02x\n",
                    dev.bus, dev.device, dev.function, vendor_id,
                    dev.header_type, dev.class_code.base, dev.class_code.sub,
                    dev.class_code.interface);
            Print(s);
        }
    } else if (command[0] != 0) {
        Print("unknown command: ");
        Print(command);
        Print("\n");
    }
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

Rectangle<int> Terminal::HistoryUpDown(int direction) {
    if (direction == -1 && cmd_history_index_ >= 0) {
        --cmd_history_index_;
    } else if (direction == 1 && cmd_history_index_ + 1 < cmd_history_.size()) {
        ++cmd_history_index_;
    }

    cursor_pos_.x = 1;
    const auto first_pos = CalcCursorPos();

    Rectangle<int> draw_area{
        first_pos, {kFontHorizonPixels * (kColumns - 1), kFontVerticalPixels}};
    FillRectangle(*window_->Drawer(), draw_area.pos, draw_area.size, {0, 0, 0});

    const char* history = "";
    if (cmd_history_index_ >= 0) {
        history = &cmd_history_[cmd_history_index_][0];
    }

    strcpy(&linebuf_[0], history);
    linebuf_index_ = strlen(history);

    WriteString(*window_->Drawer(), first_pos, history, {255, 255, 255});
    cursor_pos_.x = linebuf_index_ + 1;
    return draw_area;
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