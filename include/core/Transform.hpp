//
// Created by apolline on 12/03/2026.
//

#pragma once

#include <cstdint>
#include <memory>

#include "core/ImageBuffer.hpp"

namespace core
{
// Rotate around image center, output has SAME size.
// Pixels outside source become `bgColor` (default transparent).
std::shared_ptr<ImageBuffer> rotate(const ImageBuffer& src, float degrees,
                                    std::uint32_t bgColor = 0x00000000u);
}  // namespace core
