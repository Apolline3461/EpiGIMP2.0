/*
** EPITECH PROJECT, 2026
** EpiGIMP2.0
** File description:
** BucketFill
*/

#include "core/BucketFill.hpp"

#include <cassert>
#include <tuple>
#include <vector>

#include "core/ImageBuffer.hpp"

namespace core
{
void floodFill(ImageBuffer& buf, int startX, int startY, Color newColor)
{
    const uint32_t newCol = newColor.value;
    const int w = buf.width();
    const int h = buf.height();
    if (startX < 0 || startX >= w || startY < 0 || startY >= h)
        return;

    const uint32_t target = buf.getPixel(startX, startY);
    if (target == newCol)
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

        buf.setPixel(x, y, newCol);

        stack.emplace_back(x + 1, y);
        stack.emplace_back(x - 1, y);
        stack.emplace_back(x, y + 1);
        stack.emplace_back(x, y - 1);
    }
}

void floodFillWithinMask(ImageBuffer& buf, const ImageBuffer& mask, int startX, int startY,
                         Color newColor)
{
    const uint32_t newCol = newColor.value;
    const int w = buf.width();
    const int h = buf.height();
    assert(mask.width() == w && mask.height() == h);
    if (startX < 0 || startX >= w || startY < 0 || startY >= h)
        return;

    if ((mask.getPixel(startX, startY) & 0xFF) == 0)
        return;

    const uint32_t target = buf.getPixel(startX, startY);
    if (target == newCol)
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

        buf.setPixel(x, y, newCol);

        stack.emplace_back(x + 1, y);
        stack.emplace_back(x - 1, y);
        stack.emplace_back(x, y + 1);
        stack.emplace_back(x, y - 1);
    }
}

std::vector<std::tuple<int, int, uint32_t>> floodFillCollect(const ImageBuffer& buf, int startX,
                                                             int startY, Color newColor)
{
    std::vector<std::tuple<int, int, uint32_t>> changes;

    const int w = buf.width();
    const int h = buf.height();
    const uint32_t newCol = newColor.value;

    if (startX < 0 || startX >= w || startY < 0 || startY >= h)
        return changes;

    const uint32_t target = buf.getPixel(startX, startY);
    if (target == newCol)
        return changes;

    std::vector<uint8_t> visited(static_cast<size_t>(w) * static_cast<size_t>(h), 0);

    auto pushIfValid = [&](int x, int y, std::vector<std::pair<int, int>>& st)
    {
        if (x < 0 || x >= w || y < 0 || y >= h)
            return;
        const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x);
        if (visited[idx])
            return;
        visited[idx] = 1;
        st.emplace_back(x, y);
    };

    std::vector<std::pair<int, int>> stack;
    pushIfValid(startX, startY, stack);

    while (!stack.empty())
    {
        auto [x, y] = stack.back();
        stack.pop_back();

        const uint32_t oldColor = buf.getPixel(x, y);
        if (oldColor != target)
            continue;

        changes.emplace_back(x, y, oldColor);

        pushIfValid(x + 1, y, stack);
        pushIfValid(x - 1, y, stack);
        pushIfValid(x, y + 1, stack);
        pushIfValid(x, y - 1, stack);
    }

    return changes;
}

std::vector<std::tuple<int, int, uint32_t>> floodFillWithinMaskCollect(const ImageBuffer& buf,
                                                                       const ImageBuffer& mask,
                                                                       int startX, int startY,
                                                                       Color newColor)
{
    std::vector<std::tuple<int, int, uint32_t>> changes;

    const int w = buf.width();
    const int h = buf.height();
    assert(mask.width() == w && mask.height() == h);

    const uint32_t newCol = newColor.value;
    if (startX < 0 || startX >= w || startY < 0 || startY >= h)
        return changes;

    if ((mask.getPixel(startX, startY) & 0xFF) == 0)
        return changes;

    const uint32_t target = buf.getPixel(startX, startY);
    if (target == newCol)
        return changes;

    std::vector<uint8_t> visited(static_cast<size_t>(w) * static_cast<size_t>(h), 0);

    auto pushIfValid = [&](int x, int y, std::vector<std::pair<int, int>>& st)
    {
        if (x < 0 || x >= w || y < 0 || y >= h)
            return;
        const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x);
        if (visited[idx])
            return;
        visited[idx] = 1;
        st.emplace_back(x, y);
    };

    std::vector<std::pair<int, int>> stack;
    pushIfValid(startX, startY, stack);

    while (!stack.empty())
    {
        auto [x, y] = stack.back();
        stack.pop_back();

        if ((mask.getPixel(x, y) & 0xFF) == 0)
            continue;

        const uint32_t oldColor = buf.getPixel(x, y);
        if (oldColor != target)
            continue;

        changes.emplace_back(x, y, oldColor);

        pushIfValid(x + 1, y, stack);
        pushIfValid(x - 1, y, stack);
        pushIfValid(x, y + 1, stack);
        pushIfValid(x, y - 1, stack);
    }

    return changes;
}

}  // namespace core
