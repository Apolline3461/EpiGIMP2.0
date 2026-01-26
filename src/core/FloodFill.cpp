#include "core/FloodFill.hpp"

#include <cassert>
#include <vector>

#include "core/ImageBuffer.hpp"

namespace core
{
void floodFill(ImageBuffer& buf, int startX, int startY, uint32_t newColor)
{
    const int w = buf.width();
    const int h = buf.height();
    if (startX < 0 || startX >= w || startY < 0 || startY >= h)
        return;

    const uint32_t target = buf.getPixel(startX, startY);
    if (target == newColor)
        return;

    std::vector<std::pair<int, int>> stack;
    stack.emplace_back(startX, startY);

    while (!stack.empty())
    {
        auto [x, y] = stack.back();
        stack.pop_back();

        if (x < 0 || x >= w || y < 0 || y >= h)
            continue;

        if (buf.getPixel(x, y) != target)
            continue;

        buf.setPixel(x, y, newColor);

        stack.emplace_back(x + 1, y);
        stack.emplace_back(x - 1, y);
        stack.emplace_back(x, y + 1);
        stack.emplace_back(x, y - 1);
    }
}

void floodFillWithinMask(ImageBuffer& buf, const ImageBuffer& mask, int startX, int startY,
                         uint32_t newColor)
{
    const int w = buf.width();
    const int h = buf.height();
    assert(mask.width() == w && mask.height() == h);
    if (startX < 0 || startX >= w || startY < 0 || startY >= h)
        return;

    if ((mask.getPixel(startX, startY) & 0xFF) == 0)
        return;

    const uint32_t target = buf.getPixel(startX, startY);
    if (target == newColor)
        return;

    std::vector<std::pair<int, int>> stack;
    stack.emplace_back(startX, startY);

    while (!stack.empty())
    {
        auto [x, y] = stack.back();
        stack.pop_back();

        if (x < 0 || x >= w || y < 0 || y >= h)
            continue;

        if ((mask.getPixel(x, y) & 0xFF) == 0)
            continue;

        if (buf.getPixel(x, y) != target)
            continue;

        buf.setPixel(x, y, newColor);

        stack.emplace_back(x + 1, y);
        stack.emplace_back(x - 1, y);
        stack.emplace_back(x, y + 1);
        stack.emplace_back(x, y - 1);
    }
}

}  // namespace core
