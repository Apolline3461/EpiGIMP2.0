#pragma once

#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

class ImageBuffer;

namespace core
{
struct Color
{
    uint32_t value;
    constexpr Color(uint32_t v = 0u) noexcept : value(v) {}
};

void floodFill(ImageBuffer& buf, int startX, int startY, Color newColor);
void floodFillWithinMask(ImageBuffer& buf, const ImageBuffer& mask, int startX, int startY,
                         Color newColor);

// Optimized versions that return changed pixels (x, y, oldColor)
std::vector<std::tuple<int, int, uint32_t>> floodFillTracked(ImageBuffer& buf, int startX,
                                                             int startY, Color newColor);
std::vector<std::tuple<int, int, uint32_t>> floodFillWithinMaskTracked(ImageBuffer& buf,
                                                                       const ImageBuffer& mask,
                                                                       int startX, int startY,
                                                                       Color newColor);
}  // namespace core
