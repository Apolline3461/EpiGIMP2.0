//
// Created by apolline on 17/12/2025.
//
#include "core/Document.hpp"

#include "core/Layer.hpp"

Document::Document(const int width, const int height, const float dpi)
    : width_{width}, height_{height}, dpi_{dpi}
{
}

int Document::width() const noexcept
{
    return width_;
}

int Document::height() const noexcept
{
    return height_;
}

float Document::dpi() const noexcept
{
    return dpi_;
}

int Document::layerCount() const noexcept
{
    return static_cast<int>(layers_.size());
}

std::shared_ptr<Layer> Document::layerAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(layers_.size()))
        return nullptr;
    return layers_[static_cast<std::size_t>(index)];
}

int Document::addLayer(std::shared_ptr<Layer> layer)
{
    if (!layer)
        return -1;
    layers_.push_back(std::move(layer));
    return static_cast<int>(layers_.size()) - 1;
}

int Document::addLayer(std::shared_ptr<Layer> layer, int idx)
{
    if (!layer)
        return -1;

    if (const int size = static_cast<int>(layers_.size()); idx < 0 || idx > size)
        return -1;

    layers_.insert(layers_.begin() + idx, std::move(layer));
    return idx;
}

void Document::removeLayer(int idx)
{
    const int size = static_cast<int>(layers_.size());
    if (idx < 0 || idx >= size)
        return;
    layers_.erase(layers_.begin() + idx);
}

void Document::reorderLayer(int from, int to)
{
    const int size = static_cast<int>(layers_.size());

    if (from < 0 || from >= size || to < 0 || to >= size || from == to)
        return;

    auto tmpLayer = layers_[static_cast<std::size_t>(from)];
    layers_.erase(layers_.begin() + from);
    if (to > static_cast<int>(layers_.size()))
        to = static_cast<int>(layers_.size());
    layers_.insert(layers_.begin() + to, std::move(tmpLayer));
}

void Document::mergeDown(int from)
{
    const int size = static_cast<int>(layers_.size());

    if (from <= 0 || from >= size)
        return;
    layers_.erase(layers_.begin() + from);
}