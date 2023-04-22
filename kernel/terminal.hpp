#pragma once

#include <deque>
#include <map>

#include "layer.hpp"
#include "task.hpp"
#include "window.hpp"

class Terminal {
   public:
    static const int kRows = 15, kColumns = 60;

    Terminal();
    unsigned int LayerID() const { return layer_id_; }
    void BlinkCursor();

   private:
    std::shared_ptr<ToplevelWindow> window_;
    unsigned int layer_id_;

    Vector2D<int> cursor_pos_{0, 0};
    bool cursor_visible_{true};
    void DrawCursor(bool visible);
};

void TaskTerminal(uint64_t task_id, int64_t data);