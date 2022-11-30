#include "layer.hpp"

#include <algorithm>

Layer::Layer(unsigned int id) : id_{id} {}

unsigned int Layer::ID() const { return id_; }

Layer& Layer::SetWindow(const std::shared_ptr<Window>& window) {
    window_ = window;
    return *this;
}

std::shared_ptr<Window> Layer::GetWindow() const { return window_; }

Layer& Layer::Move(Vector2D<int> pos) {
    pos_ = pos;
    return *this;
}

Layer& Layer::MoveRelative(Vector2D<int> pos_diff) {
    pos_ += pos_diff;
    return *this;
}

void Layer::DrawTo(ScreenDrawer& drawer) const {
    if (window_) {
        window_->DrawTo(drawer, pos_);
    }
}

void LayerManager::SetDrawer(ScreenDrawer* drawer) { drawer_ = drawer; }

Layer& LayerManager::NewLayer() {
    ++latest_id_;
    return *layers_.emplace_back(new Layer{latest_id_});
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_position) {
    FindLayer(id)->Move(new_position);
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff) {
    FindLayer(id)->MoveRelative(pos_diff);
}

void LayerManager::Draw() const {
    for (auto layer : layer_stack_) {
        layer->DrawTo(*drawer_);
    }
}

void LayerManager::Hide(unsigned int id) {}

Layer* LayerManager::FindLayer(unsigned int id) {
    auto pred = [id](const std::unique_ptr<Layer>& elem) {
        return elem->ID() == id;
    };

    auto it = std::find_if(layers_.begin(), layers_.end(), pred);
    if (it == layers_.end()) {
        return nullptr;
    }

    return it->get();
}