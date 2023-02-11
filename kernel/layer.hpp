#pragma once

#include <map>
#include <memory>
#include <vector>

#include "drawing.hpp"
#include "window.hpp"

class Layer {
   public:
    Layer(unsigned int id = 0);
    unsigned int ID() const;

    Layer& SetWindow(const std::shared_ptr<Window>& window);
    std::shared_ptr<Window> GetWindow();

    Layer& Move(Vector2D<int> pos);
    Layer& MoveRelative(Vector2D<int> pos_diff);
    void DrawTo(ScreenDrawer& drawer) const;

   private:
    unsigned int id_;
    Vector2D<int> pos_;
    std::shared_ptr<Window> window_;
};

class LayerManager {
   public:
    void SetDrawer(ScreenDrawer* drawer);
    Layer& NewLayer();

    void Draw() const;
    void Move(unsigned int id, Vector2D<int> new_position);
    void MoveRelative(unsigned int id, Vector2D<int> pos_diff);

    void UpDown(unsigned int id, int new_height);
    void Hide(unsigned int id);

   private:
    ScreenDrawer* drawer_{nullptr};
    // 全てのレイヤを格納する動的配列
    std::vector<std::unique_ptr<Layer>> layers_{};
    // レイヤの重なりを表す配列 先頭が最背面となり、末尾が最前面となる
    std::vector<Layer*> layer_stack_{};
    unsigned int latest_id_{0};

    Layer* FindLayer(unsigned int id);
};

extern LayerManager* layer_manager;