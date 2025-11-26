//
// Created by apolline on 17/11/2025.
//

#include "../../include/core/ImageBuffer.h"

ImageBuffer::ImageBuffer(const int width, const int height) : width_(width), height_(height)
{
    assert(width_ > 0 && height_ > 0);
    stride_ = width_ * 4;
    rgbaPixels_.resize(height_ * stride_);
    fill(0x00000000u);
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

void ImageBuffer::fill(uint32_t rgba)
{
    const auto r = static_cast<uint8_t>((rgba >> 24) & 0xFF);
    const auto g = static_cast<uint8_t>((rgba >> 16) & 0xFF);
    const auto b = static_cast<uint8_t>((rgba >> 8) & 0xFF);
    const auto a = static_cast<uint8_t>(rgba & 0xFF);

    for (int y = 0; y < height_; ++y)
    {
        for (int x = 0; x < width_; ++x)
        {
            const int offset = y * stride_ + x * 4;
            rgbaPixels_[offset + 0] = r;
            rgbaPixels_[offset + 1] = g;
            rgbaPixels_[offset + 2] = b;
            rgbaPixels_[offset + 3] = a;
        }
    }
}

uint32_t ImageBuffer::getPixel(const int x, const int y) const
{
    assert(x >= 0 && x < width_ && y >= 0 && y < height_);
    const int offset = y * stride_ + x * 4;

    const uint8_t r = rgbaPixels_[offset + 0];
    const uint8_t g = rgbaPixels_[offset + 1];
    const uint8_t b = rgbaPixels_[offset + 2];
    const uint8_t a = rgbaPixels_[offset + 3];

    return (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
}

void ImageBuffer::setPixel(const int x, const int y, const uint32_t rgba)
{
    assert(x >= 0 && x < width_ && y >= 0 && y < height_);
    const int offset = y * stride_ + x * 4;

    const auto r = static_cast<uint8_t>((rgba >> 24) & 0xFF);
    const auto g = static_cast<uint8_t>((rgba >> 16) & 0xFF);
    const auto b = static_cast<uint8_t>((rgba >> 8) & 0xFF);
    const auto a = static_cast<uint8_t>(rgba & 0xFF);

    rgbaPixels_[offset + 0] = r;
    rgbaPixels_[offset + 1] = g;
    rgbaPixels_[offset + 2] = b;
    rgbaPixels_[offset + 3] = a;
}