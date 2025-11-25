#include "layer.hpp"

#include <utility>

#include "imagebuffer.hpp"

// fake Layer class for testing purposes
Layer::Layer() = default;

Layer::Layer(std::string ID, std::string Name, int w, int h)
    : id(std::move(ID)), name(std::move(Name)), image(std::make_unique<ImageBuffer>(w, h))
{
}
Layer::~Layer() = default;
