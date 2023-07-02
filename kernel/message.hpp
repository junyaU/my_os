#pragma once

enum class LayerOperation { Move, MoveRelative, Draw, DrawArea };

struct Message {
    enum Type {
        kInterruptXHCI,
        kTimerTimeout,
        kKeyPush,
        kLayer,
        kLayerFinish,
        kMouseMove,
        kMouseButton,
    } type;

    uint64_t source_task;

    union {
        struct {
            unsigned long timeout;
            int value;
        } timer;

        struct {
            uint8_t modifier;
            uint8_t keycode;
            char ascii;
        } keyboard;

        struct {
            LayerOperation op;
            unsigned int layer_id;
            int x, y;
            int w, h;
        } layer;

        struct {
            int x, y;
            int dx, dy;
            uint8_t buttons;
        } mouse_move;

        struct {
            int x, y;
            int press;
            int button;
        } mouse_button;
    } arg;
};
