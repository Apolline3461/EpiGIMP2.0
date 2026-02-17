//
// Created by apolline on 03/02/2026.
//

#pragma once
#include <cstdint>

#include "common/Colors.hpp"

namespace app
{

enum class ToolKind : std::uint8_t
{
    Pencil,
    Brush,
};

struct ToolParams
{
    ToolKind tool = ToolKind::Pencil;
    std::uint32_t color = common::colors::Black;
    int size = 1;
    float hardness = 1.0f; /* 0.0 = soft edge, 1.0 = hard edge */
};

}  // namespace app
