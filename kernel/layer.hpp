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
    Vector2D<int> GetPosition() const;

    Layer& Move(Vector2D<int> pos);
    Layer& MoveRelative(Vector2D<int> pos_diff);
    void DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const;

   private:
    unsigned int id_;
    Vector2D<int> pos_;
    std::shared_ptr<Window> window_;
};

class LayerManager {
   public:
    void SetDrawer(FrameBuffer* screen);
    Layer& NewLayer();

    void Draw(const Rectangle<int>& area) const;
    void Draw(unsigned int id) const;
    void Move(unsigned int id, Vector2D<int> new_position);
    void MoveRelative(unsigned int id, Vector2D<int> pos_diff);

    void UpDown(unsigned int id, int new_height);
    void Hide(unsigned int id);

   private:
    FrameBuffer* screen_{nullptr};
    // 全てのレイヤを格納する動的配列
    std::vector<std::unique_ptr<Layer>> layers_{};
    // レイヤの重なりを表す配列 先頭が最背面となり、末尾が最前面となる
    std::vector<Layer*> layer_stack_{};
    unsigned int latest_id_{0};

    Layer* FindLayer(unsigned int id);
};

extern LayerManager* layer_manager;