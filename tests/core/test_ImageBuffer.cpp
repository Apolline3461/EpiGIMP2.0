//
// Created by apolline on 17/11/2025.
//
#include <gtest/gtest.h>
#include "core/ImageBuffer.hpp"

TEST(ImageBufferTest, HasCorrectDimensions)
{
    const ImageBuffer buffer{3,6};

    EXPECT_EQ(buffer.width(), 3);
    EXPECT_EQ(buffer.height(), 6);
    EXPECT_EQ(buffer.strideBytes(), 3 * 4); //4 bytes by pix
}

TEST(ImageBufferTest, NewBufferIsZeroInitialized)
{
    const ImageBuffer buffer{3, 2};

    for (int y = 0; y < buffer.height(); ++y) {
        for (int x = 0; x < buffer.width(); ++x)
            EXPECT_EQ(buffer.getPixel(x, y), 0u);
    }
}

TEST(ImageBufferTest, SetAndgetPixelOnCorners)
{
    ImageBuffer buf{2, 2};
    const uint32_t c1 = 0xFF0000FFu;
    const uint32_t c2 = 0x00FF00FFu;

    buf.setPixel(0, 0, c1);
    buf.setPixel(1, 1, c2);

    EXPECT_EQ(buf.getPixel(0, 0), c1);
    EXPECT_EQ(buf.getPixel(1, 1), c2);
    EXPECT_EQ(buf.getPixel(1, 0), 0u);
    EXPECT_EQ(buf.getPixel(0, 1), 0u);
}

TEST(ImageBufferTest, SinglePixelDimensionsAndAccess)
{
    ImageBuffer buffer{1, 1};
    constexpr uint32_t color = 0x12345678u;

    EXPECT_EQ(buffer.width(), 1);
    EXPECT_EQ(buffer.height(), 1);
    EXPECT_EQ(buffer.strideBytes(), 4);

    buffer.setPixel(0, 0, color);
    EXPECT_EQ(buffer.getPixel(0, 0), color);
}

TEST(ImageBufferTest, FillAllPixels)
{
    ImageBuffer buffer{3,6};
    constexpr uint32_t red = 0xFF0000FFu;

    buffer.fill(red);

    for (int y = 0; y < buffer.height(); ++y) {
        for (int x = 0; x < buffer.width(); ++x) {
            EXPECT_EQ(buffer.getPixel(x, y), red);
        }
    }
}

TEST(ImageBufferTest, DataPointerIsNotNullAndConsistent)
{
    ImageBuffer buffer{2, 2};
    auto* p1 = buffer.data();
    auto* p2 = buffer.data();

    EXPECT_NE(p1, nullptr);
    EXPECT_EQ(p1, p2);
}