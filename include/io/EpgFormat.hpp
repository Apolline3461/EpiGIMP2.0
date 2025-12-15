#pragma once

#include <string>

#include "core/ImageBuffer.h"

namespace EpgFormat
{
bool save(const std::string& fileName, const ImageBuffer& image);
bool load(const std::string& fileName, ImageBuffer& outImage);
}  // namespace EpgFormat
