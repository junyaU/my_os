#pragma once

#include <map>
#include <memory>
#include <vector>

#include "drawing.hpp"
#include "message.hpp"
#include "window.hpp"

class Layer {
   public:
    Layer(unsigned int id = 0);
    unsigned int ID() const;

    Layer& SetWindow(const std::shared_ptr<Window>& window);
    std::shared_ptr<Window> GetWindow();
    Vector2D<int> GetPosition() const;
    Layer& SetDraggable(bool draggable);
    bool IsDraggable() const;

    Layer& Move(Vector2D<int> pos);
    Layer& MoveRelative(Vector2D<int> pos_diff);
    void DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const;

   private:
    unsigned int id_;
    Vector2D<int> pos_;
    std::shared_ptr<Window> window_;
    bool draggable_{false};
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
    Layer* FindLayerByPoisition(Vector2D<int> pos,
                                unsigned int exclude_id) const;
    Layer* FindLayer(unsigned int id);
    int GetHeight(unsigned int id);

   private:
    FrameBuffer* screen_{nullptr};
    mutable FrameBuffer back_buffer_{};
    // 全てのレイヤを格納する動的配列
    std::vector<std::unique_ptr<Layer>> layers_{};
    // レイヤの重なりを表す配列 先頭が最背面となり、末尾が最前面となる
    std::vector<Layer*> layer_stack_{};
    unsigned int latest_id_{0};
};

extern LayerManager* layer_manager;

class ActiveLayer {
   public:
    ActiveLayer(LayerManager& manager);
    void SetMouseLayer(unsigned int mouse_layer);
    void Activate(unsigned int layer_id);
    unsigned int GetActiveLayer() const { return active_layer_; }

   private:
    LayerManager& manager_;
    unsigned int active_layer_{0};
    unsigned int mouse_layer_{0};
};

extern ActiveLayer* active_layer;

void InitializeLayer();

void ProcessLayerMessage(const Message& msg);