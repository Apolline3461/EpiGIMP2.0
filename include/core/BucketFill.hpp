#pragma once

#include <cstdint>

class ImageBuffer;

namespace core
{
void floodFill(ImageBuffer& buf, int startX, int startY, uint32_t newColor);
void floodFillWithinMask(ImageBuffer& buf, const ImageBuffer& mask, int startX, int startY,
                         uint32_t newColor);
}  // namespace core
