/*
** EPITECH PROJECT, 2026
** EpiGIMP2.0
** File description:
** Selection
*/

#include "core/Selection.hpp"

#include <cassert>

#include "core/ImageBuffer.h"

uint8_t Selection::t_at(const int x, const int y) const
{
    if (!hasMask())
        return 0u;

    if (x < 0 || y < 0 || x >= mask_->width() || y >= mask_->height())
        return 0u;

    const uint32_t px = mask_->getPixel(x, y);
    return static_cast<uint8_t>(px & 0xFF);
}

void Selection::addRect(const int x, const int y, const int w, const int h,
                        std::shared_ptr<ImageBuffer> reference)
{
    if (!mask_)
    {
        if (!reference)
            return;
        mask_ = std::make_shared<ImageBuffer>(reference->width(), reference->height());
        mask_->fill(0x00000000u);
    }

    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(mask_->width(), x + w);
    const int y1 = std::min(mask_->height(), y + h);

    for (int yy = y0; yy < y1; ++yy)
    {
        for (int xx = x0; xx < x1; ++xx)
        {
            mask_->setPixel(xx, yy, 0x000000FFu);
        }
    }
}

void Selection::subtractRect(const int x, const int y, const int w, const int h)
{
    if (!mask_)
        return;

    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(mask_->width(), x + w);
    const int y1 = std::min(mask_->height(), y + h);

    for (int yy = y0; yy < y1; ++yy)
    {
        for (int xx = x0; xx < x1; ++xx)
        {
            mask_->setPixel(xx, yy, 0x00000000u);
        }
    }
}
