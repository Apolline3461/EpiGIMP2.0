//
// Created by apolline on 17/11/2025.
//

#include "../../include/core/ImageBuffer.h"
ImageBuffer::ImageBuffer(int width, int height) : width_(width), height_(height)
{
    stride_ = width_ * 4;
}

int ImageBuffer::width() const noexcept
{
    return 0;
}

int ImageBuffer::height() const noexcept
{
    return 0;
}

int ImageBuffer::strideBytes() const noexcept
{
    return 0;
}
uint8_t* ImageBuffer::dawta() noexcept
{
    return nullptr;
}
const uint8_t* ImageBuffer::data() const noexcept
{
    return nullptr;
}
void ImageBuffer::fill(uint32_t rgba) {}
uint32_t ImageBuffer::getPixel(int x, int y) const
{
    return 0;
}
void ImageBuffer::setPixel(int x, int y, uint32_t rgba) {}