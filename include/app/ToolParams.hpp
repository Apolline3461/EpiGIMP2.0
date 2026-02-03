//
// Created by apolline on 03/02/2026.
//

#pragma once
#include <cstdint>

#include "common/Colors.hpp"

namespace app
{

enum class ToolKind
{
    Pencil,
};

struct ToolParams
{
    ToolKind tool = ToolKind::Pencil;
    std::uint32_t color = common::colors::Black;
    int size = 1;
};

}  // namespace app
