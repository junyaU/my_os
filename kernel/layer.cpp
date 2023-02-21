#include "layer.hpp"

#include <algorithm>

Layer::Layer(unsigned int id) : id_{id} {}

unsigned int Layer::ID() const { return id_; }

Layer& Layer::SetWindow(const std::shared_ptr<Window>& window) {
    window_ = window;
    return *this;
}

std::shared_ptr<Window> Layer::GetWindow() { return window_; }

Layer& Layer::Move(Vector2D<int> pos) {
    pos_ = pos;
    return *this;
}

Layer& Layer::MoveRelative(Vector2D<int> pos_diff) {
    pos_ += pos_diff;
    return *this;
}

void Layer::DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const {
    if (window_) {
        window_->DrawTo(screen, pos_, area);
    }
}

void LayerManager::SetDrawer(FrameBuffer* screen) { screen_ = screen; }

Layer& LayerManager::NewLayer() {
    ++latest_id_;
    return *layers_.emplace_back(new Layer{latest_id_});
}

void LayerManager::Draw(const Rectangle<int>& area) const {
    for (auto layer : layer_stack_) {
        layer->DrawTo(*screen_, area);
    }
}

void LayerManager::Draw(unsigned int id, Vector2D<int> new_pos) const {}

void LayerManager::Move(unsigned int id, Vector2D<int> new_position) {
    FindLayer(id)->Move(new_position);
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff) {
    FindLayer(id)->MoveRelative(pos_diff);
}

void LayerManager::UpDown(unsigned int id, int new_height) {
    if (new_height < 0) {
        Hide(id);
        return;
    }

    if (new_height > layer_stack_.size()) {
        new_height = layer_stack_.size();
    }

    auto layer = FindLayer(id);
    auto old_pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
    auto new_pos = layer_stack_.begin() + new_height;

    if (old_pos == layer_stack_.end()) {
        layer_stack_.insert(new_pos, layer);
        return;
    }

    if (new_pos == layer_stack_.end()) {
        --new_pos;
    }
    layer_stack_.erase(old_pos);
    layer_stack_.insert(new_pos, layer);
}

void LayerManager::Hide(unsigned int id) {
    auto layer = FindLayer(id);
    auto pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
    if (pos != layer_stack_.end()) {
        layer_stack_.erase(pos);
    }
}

Layer* LayerManager::FindLayer(unsigned int id) {
    auto pred = [id](const std::unique_ptr<Layer>& el) {
        return el->ID() == id;
    };

    auto target = std::find_if(layers_.begin(), layers_.end(), pred);
    if (target == layers_.end()) {
        return nullptr;
    }

    return target->get();
}

LayerManager* layer_manager;