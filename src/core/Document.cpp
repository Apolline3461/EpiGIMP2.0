//
// Created by apolline on 17/12/2025.
//
#include "core/Document.hpp"

#include "core/Layer.hpp"
Document::Document(const int width, const int height, const float dpi)
    : width_{width}, height_{height}, dpi_{dpi}
{
    if (width_ <= 0 || height_ <= 0)
        throw std::invalid_argument("Document: invalid size");
    if (dpi_ <= 0.f)
        throw std::invalid_argument("Document: invalid dpi");
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

size_t Document::layerCount() const noexcept
{
    return static_cast<int>(layers_.size());
}

std::shared_ptr<Layer> Document::layerAt(const size_t index) const
{
    if (index >= layers_.size())
        return nullptr;
    return layers_[index];
}

size_t Document::addLayer(std::shared_ptr<Layer> layer)
{
    if (!layer)
        return -1;
    layers_.push_back(std::move(layer));
    return layers_.size() - 1;
}

size_t Document::addLayer(std::shared_ptr<Layer> layer, const size_t idx)
{
    if (!layer)
        return -1;

    if (idx > layers_.size())
        return -1;

    layers_.insert(layers_.begin() + static_cast<std::ptrdiff_t>(idx), std::move(layer));
    return idx;
}

void Document::removeLayer(size_t idx)
{
    const auto size = layers_.size();
    if (idx >= size)
        return;
    layers_.erase(layers_.begin() + static_cast<std::ptrdiff_t>(idx));
}

void Document::reorderLayer(size_t from, size_t to)
{
    const auto size = layers_.size();

    if (from >= size || to >= size || from == to)
        return;

    auto tmpLayer = layers_[from];
    layers_.erase(layers_.begin() + static_cast<std::ptrdiff_t>(from));
    if (to > layers_.size())
        to = layers_.size();
    layers_.insert(layers_.begin() + static_cast<std::ptrdiff_t>(to), std::move(tmpLayer));
}

void Document::mergeDown(const size_t from)
{
    const auto size = layers_.size();

    if (from <= 0 || from >= size)
        return;
    layers_.erase(layers_.begin() + static_cast<std::ptrdiff_t>(from));
}