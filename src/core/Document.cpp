//
// Created by apolline on 17/12/2025.
//
#include "core/Document.hpp"

#include <algorithm>
#include <cmath>

#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

Document::Document(const int width, const int height,
                   const float dpi)  // NOLINT(bugprone-easily-swappable-parameters)
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
    // Merge layer 'from' onto layer 'from-1' (src over dst)
    auto srcLayer = layers_[static_cast<std::size_t>(from)];
    auto dstLayer = layers_[static_cast<std::size_t>(from - 1)];
    if (srcLayer && dstLayer && srcLayer->image() && dstLayer->image())
    {
        auto srcImg = srcLayer->image();
        auto dstImg = dstLayer->image();
        const int maxW = std::min(srcImg->width(), dstImg->width());
        const int maxH = std::min(srcImg->height(), dstImg->height());

        // Local strong type to avoid clang-tidy easily-swappable-parameters false positives
        struct LayerOpacity
        {
            explicit LayerOpacity(float v) : value(v) {}
            float value;
        };

        auto blendPixel = [](std::uint32_t src, std::uint32_t dst,
                             LayerOpacity layerOpacity)
            -> std::uint32_t  // NOLINT(bugprone-easily-swappable-parameters)
        {
            auto extract = [](std::uint32_t px, int shift) -> std::uint8_t
            { return static_cast<std::uint8_t>((px >> shift) & 0xFFu); };
            const float srcR = static_cast<float>(extract(src, 24)) / 255.0f;
            const float srcG = static_cast<float>(extract(src, 16)) / 255.0f;
            const float srcB = static_cast<float>(extract(src, 8)) / 255.0f;
            const float srcA = static_cast<float>(extract(src, 0)) / 255.0f;

            const float dstR = static_cast<float>(extract(dst, 24)) / 255.0f;
            const float dstG = static_cast<float>(extract(dst, 16)) / 255.0f;
            const float dstB = static_cast<float>(extract(dst, 8)) / 255.0f;
            const float dstA = static_cast<float>(extract(dst, 0)) / 255.0f;

            const float effA = srcA * std::clamp(layerOpacity.value, 0.0f, 1.0f);
            const float outA = effA + dstA * (1.0f - effA);
            if (outA <= 0.0f)
                return 0u;

            const float outR = (srcR * effA + dstR * dstA * (1.0f - effA)) / outA;
            const float outG = (srcG * effA + dstG * dstA * (1.0f - effA)) / outA;
            const float outB = (srcB * effA + dstB * dstA * (1.0f - effA)) / outA;

            auto toByte = [](float v) -> std::uint8_t
            {
                float clamped = std::clamp(v, 0.0f, 1.0f);
                return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
            };

            return (static_cast<std::uint32_t>(toByte(outR)) << 24) |
                   (static_cast<std::uint32_t>(toByte(outG)) << 16) |
                   (static_cast<std::uint32_t>(toByte(outB)) << 8) |
                   static_cast<std::uint32_t>(toByte(outA));
        };

        const float opacity = srcLayer->opacity();
        for (int y = 0; y < maxH; ++y)
        {
            for (int x = 0; x < maxW; ++x)
            {
                const std::uint32_t s = srcImg->getPixel(x, y);
                const std::uint32_t d = dstImg->getPixel(x, y);
                const std::uint32_t blended = blendPixel(s, d, LayerOpacity(opacity));
                dstImg->setPixel(x, y, blended);
            }
        }
    }

    layers_.erase(layers_.begin() + from);
}

void Document::setLayers(std::vector<std::shared_ptr<Layer>> layers)
{
    layers_ = std::move(layers);
}