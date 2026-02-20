#pragma once

#include <cstdint>
#include <tuple>
#include <vector>

class ImageBuffer;

namespace core
{
struct Color
{
    uint32_t value;
    explicit constexpr Color(uint32_t v = 0u) noexcept : value(v) {}
};

void floodFill(ImageBuffer& buf, int startX, int startY, Color newColor);
void floodFillWithinMask(ImageBuffer& buf, const ImageBuffer& mask, int startX, int startY,
                         Color newColor);

// Collect-only: returns (x,y,oldColor) without mutating buf
std::vector<std::tuple<int, int, uint32_t>> floodFillCollect(const ImageBuffer& buf, int startX,
                                                             int startY, Color newColor);

std::vector<std::tuple<int, int, uint32_t>> floodFillWithinMaskCollect(const ImageBuffer& buf,
                                                                       const ImageBuffer& mask,
                                                                       int startX, int startY,
                                                                       Color newColor);
}  // namespace core
