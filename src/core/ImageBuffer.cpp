//
// Created by apolline on 17/11/2025.
//

#include "../../include/core/ImageBuffer.h"
ImageBuffer::ImageBuffer(const int width, const int height) : width_(width), height_(height)
{
    stride_ = width_ * 4;
}

int ImageBuffer::width() const noexcept
{
    return width_;
}

int ImageBuffer::height() const noexcept
{
    return height_;
}

int ImageBuffer::strideBytes() const noexcept
{
    return stride_;
}

uint8_t* ImageBuffer::data() noexcept
{
    return rgbaPixels_.data();
}
const uint8_t* ImageBuffer::data() const noexcept
{
    return rgbaPixels_.data();
}
void ImageBuffer::fill(uint32_t rgba) {}
uint32_t ImageBuffer::getPixel(int x, int y) const
{
    return 0;
}
void ImageBuffer::setPixel(int x, int y, uint32_t rgba) {}