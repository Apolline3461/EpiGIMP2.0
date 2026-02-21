/*
** EPITECH PROJECT, 2026
** EpiGIMP2.0
** File description:
** Selection
*/

#include "core/Selection.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>

#include "core/ImageBuffer.hpp"

uint8_t Selection::t_at(const int x, const int y) const
{
    if (!hasMask())
        return 0u;

    if (x < 0 || y < 0 || x >= mask_->width() || y >= mask_->height())
        return 0u;

    const uint32_t px = mask_->getPixel(x, y);
    return static_cast<uint8_t>(px & 0xFF);
}

void Selection::addRect(const Selection::Rect& rect, const std::shared_ptr<ImageBuffer>& reference)
{
    if (!mask_)
    {
        if (!reference)
            return;
        mask_ = std::make_shared<ImageBuffer>(reference->width(), reference->height());
        mask_->fill(0x00000000u);
    }

    const int x = rect.x;
    const int y = rect.y;
    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(mask_->width(), x + rect.w);
    const int y1 = std::min(mask_->height(), y + rect.h);

    for (int yy = y0; yy < y1; ++yy)
    {
        for (int xx = x0; xx < x1; ++xx)
        {
            mask_->setPixel(xx, yy, 0x000000FFu);
        }
    }
}

void Selection::subtractRect(const Selection::Rect& rect)
{
    if (!mask_)
        return;

    const int x = rect.x;
    const int y = rect.y;
    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(mask_->width(), x + rect.w);
    const int y1 = std::min(mask_->height(), y + rect.h);

    for (int yy = y0; yy < y1; ++yy)
    {
        for (int xx = x0; xx < x1; ++xx)
        {
            mask_->setPixel(xx, yy, 0x00000000u);
        }
    }
}

std::optional<Selection::Rect> Selection::boundingRect() const
{
    if (!mask_)
        return std::nullopt;

    const int w = mask_->width();
    const int h = mask_->height();

    int minX = w, minY = h;
    int maxX = -1, maxY = -1;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            if (t_at(x, y) != 0)
            {
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }

    if (maxX < minX || maxY < minY)
        return std::nullopt;

    // Rect: x,y,w,h  (w/h inclusifs -> +1)
    return Rect{minX, minY, (maxX - minX + 1), (maxY - minY + 1)};
}
