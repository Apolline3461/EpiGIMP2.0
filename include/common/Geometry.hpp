//
// Created by apolline on 02/02/2026.
//

#pragma once
#include <cstdint>

namespace common
{
struct Point
{
    int x{};
    int y{};
};

struct Rect
{
    mutable int x{};
    mutable int y{};
    mutable int w{};
    mutable int h{};
};
}  // namespace common
