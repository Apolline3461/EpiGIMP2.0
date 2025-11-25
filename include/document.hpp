#pragma once
#include <vector>
#include <memory>

#include "layer.hpp"

struct Document
{
    int width = 800;
    int height = 600;
    float dpi = 72.f;

    std::vector<std::shared_ptr<Layer>> layers;

    Document() = default;
};
