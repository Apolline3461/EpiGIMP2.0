#pragma once

#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

class ImageBuffer;

namespace core
{
void floodFill(ImageBuffer& buf, int startX, int startY, uint32_t newColor);
void floodFillWithinMask(ImageBuffer& buf, const ImageBuffer& mask, int startX, int startY,
                         uint32_t newColor);

// Optimized versions that return changed pixels (x, y, oldColor)
std::vector<std::tuple<int, int, uint32_t>> floodFillTracked(ImageBuffer& buf, int startX,
                                                             int startY, uint32_t newColor);
std::vector<std::tuple<int, int, uint32_t>> floodFillWithinMaskTracked(ImageBuffer& buf,
                                                                       const ImageBuffer& mask,
                                                                       int startX, int startY,
                                                                       uint32_t newColor);
}  // namespace core
