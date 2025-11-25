#pragma once
#include <vector>
#include <cstdint>

struct ImageBuffer
{
    int width = 0;
    int height = 0;
    int stride = 0; // = width * 4
    std::vector<uint8_t> rgba; // RGBA8 unorm

    ImageBuffer() = default;
};
