//
// Created by apolline on 25/11/2025.
//

#include "../../include/core/Layer.h"

#include <cassert>
#include <utility>

Layer::Layer(const uint64_t id, string name, shared_ptr<ImageBuffer> image, bool visible, bool locked,
             float opacity)
    : id_{id}, name_{move(name)}, visible_{visible}, locked_{locked}, opacity_{opacity}, image_{move(image)} {
}
std::uint64_t Layer::id() const noexcept
{
    return 0;
}
const std::string& Layer::name() const noexcept
{
    return name_;
}
void Layer::setName(std::string name) {}

bool Layer::visible() const noexcept
{
    return false;
}

void Layer::setVisible(bool visible) {}

bool Layer::locked() const noexcept
{
    return true;
}

void Layer::setLocked(bool locked) {}

bool Layer::isEditable() const noexcept
{
    return true;
}

float Layer::opacity() const noexcept
{
    return 0;
}
void Layer::setOpacity(float opacity) {}

const shared_ptr<ImageBuffer>& Layer::image() const noexcept
{
    return image_;
}

void Layer::setPixels(std::shared_ptr<ImageBuffer> image) {}