#pragma once

#include <string>

#include "core/ImageBuffer.hpp"

namespace EpgFormat
{
const int32_t MAX_DIM = 10000;
const int64_t MAX_PIXELS = static_cast<int64_t>(MAX_DIM) * static_cast<int64_t>(MAX_DIM);

bool save(const std::string& fileName, const ImageBuffer& image);
bool load(const std::string& fileName, ImageBuffer& outImage);
}  // namespace EpgFormat
