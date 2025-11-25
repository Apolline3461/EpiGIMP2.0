#pragma once
#include <memory>
#include <string>

#include "imagebuffer.hpp"

class Layer
{
   public:
    const uint64_t id_;  // défini à la construction
    std::string name;
    bool visible = true;
    bool locked = false;
    float opacity = 1.f;
    std::shared_ptr<ImageBuffer> pixels;

   public:
    Layer(uint64_t id) : id_(id) {}
};
