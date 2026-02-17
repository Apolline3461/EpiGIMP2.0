//
// Created by apolline on 17/12/2025.
//

#include "core/Document.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Document::Document(const int width, const int height,
                   const float dpi)  // NOLINT(bugprone-easily-swappable-parameters)
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

void Document::mergeDown(const int from)
{
    const auto size = layers_.size();

    if (from == 0 || from >= size)
        return;
    // Merge layer 'from' onto layer 'from-1' (src over dst)
    auto srcLayer = layers_[static_cast<std::size_t>(from)];
    auto dstLayer = layers_[static_cast<std::size_t>(from - 1)];
    if (srcLayer && dstLayer && srcLayer->image() && dstLayer->image())
    {
        auto srcImg = srcLayer->image();
        auto dstImg = dstLayer->image();

        // Get layer offsets
        const int srcOffsetX = srcLayer->offsetX();
        const int srcOffsetY = srcLayer->offsetY();
        const int dstOffsetX = dstLayer->offsetX();
        const int dstOffsetY = dstLayer->offsetY();

        // Calculate bounding box in document coordinates
        const int srcX0 = srcOffsetX;
        const int srcY0 = srcOffsetY;
        const int srcX1 = srcOffsetX + srcImg->width();
        const int srcY1 = srcOffsetY + srcImg->height();

        const int dstX0 = dstOffsetX;
        const int dstY0 = dstOffsetY;
        const int dstX1 = dstOffsetX + dstImg->width();
        const int dstY1 = dstOffsetY + dstImg->height();

        // Find intersection of the two layer bounds
        const int minX = std::max(srcX0, dstX0);
        const int minY = std::max(srcY0, dstY0);
        const int maxX = std::min(srcX1, dstX1);
        const int maxY = std::min(srcY1, dstY1);

        // Only blend if there's an overlap
        if (minX < maxX && minY < maxY)
        {
            // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
            auto blendPixel = [](std::uint32_t src, std::uint32_t dst,
                                 float layerOpacity)
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

                const float effA = srcA * std::clamp(layerOpacity, 0.0f, 1.0f);
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
            // Iterate over the intersection in document coordinates
            for (int docY = minY; docY < maxY; ++docY)
            {
                for (int docX = minX; docX < maxX; ++docX)
                {
                    // Convert document coordinates to layer-local coordinates
                    const int srcX = docX - srcOffsetX;
                    const int srcY = docY - srcOffsetY;
                    const int dstX = docX - dstOffsetX;
                    const int dstY = docY - dstOffsetY;

                    const std::uint32_t s = srcImg->getPixel(srcX, srcY);
                    const std::uint32_t d = dstImg->getPixel(dstX, dstY);
                    const std::uint32_t blended = blendPixel(s, d, opacity);
                    dstImg->setPixel(dstX, dstY, blended);
                }
            }
        }
    }

    layers_.erase(layers_.begin() + from);
}

// void Document::setLayers(std::vector<std::shared_ptr<Layer>> layers)
// {
//     layers_ = std::move(layers);
//     layers_.erase(layers_.begin() + static_cast<std::ptrdiff_t>(from));
// }
