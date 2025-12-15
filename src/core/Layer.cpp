//
// Created by apolline on 25/11/2025.
//

#include "core/Layer.h"

#include <cassert>
#include <utility>

Layer::Layer(const uint64_t id, std::string name, std::shared_ptr<ImageBuffer> image, bool visible,
             bool locked, float opacity)
    : id_{id},
      name_{std::move(name)},
      visible_{visible},
      locked_{locked},
      opacity_{opacity},
      image_{std::move(image)}
{
}
std::uint64_t Layer::id() const noexcept
{
    return id_;
}
const std::string& Layer::name() const noexcept
{
    return name_;
}
void Layer::setName(std::string name)
{
    name_ = std::move(name);
}

bool Layer::visible() const noexcept
{
    return visible_;
}

void Layer::setVisible(const bool visible)
{
    visible_ = visible;
}

bool Layer::locked() const noexcept
{
    return locked_;
}

void Layer::setLocked(const bool locked)
{
    locked_ = locked;
}

bool Layer::isEditable() const noexcept
{
    return !locked_;
}

float Layer::opacity() const noexcept
{
    return opacity_;
}
void Layer::setOpacity(const float opacity)
{
    if (opacity < 0.f)
        opacity_ = 0.f;
    else if (opacity > 1.f)
        opacity_ = 1.f;
    else
        opacity_ = opacity;
}

const std::shared_ptr<ImageBuffer>& Layer::image() const noexcept
{
    return image_;
}

void Layer::setImageBuffer(std::shared_ptr<ImageBuffer> image)
{
    image_ = std::move(image);
}