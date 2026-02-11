// Created by copilot on 28/01/2026

#include <QImage>

#include "ui/ImageConversion.hpp"

#include <gtest/gtest.h>

using namespace ImageConversion;

TEST(ImageConversionTest, ConvertSameSizePreservesPixels)
{
    QImage src(3, 2, QImage::Format_ARGB32);
    src.fill(qRgba(10, 20, 30, 40));

    auto buf = qImageToImageBuffer(src, 3, 2);

    EXPECT_EQ(buf.width(), 3);
    EXPECT_EQ(buf.height(), 2);

    const uint32_t expected = (static_cast<uint32_t>(10) << 24) |
                              (static_cast<uint32_t>(20) << 16) | (static_cast<uint32_t>(30) << 8) |
                              static_cast<uint32_t>(40);

    for (int y = 0; y < buf.height(); ++y)
        for (int x = 0; x < buf.width(); ++x)
            EXPECT_EQ(buf.getPixel(x, y), expected);
}

TEST(ImageConversionTest, ConvertLargerBufferLeavesExtraTransparent)
{
    QImage src(2, 1, QImage::Format_ARGB32);
    src.setPixel(0, 0, qRgba(1, 2, 3, 4));
    src.setPixel(1, 0, qRgba(5, 6, 7, 8));

    // request a bigger buffer than source image
    auto buf = qImageToImageBuffer(src, 4, 3);

    EXPECT_EQ(buf.width(), 4);
    EXPECT_EQ(buf.height(), 3);

    const uint32_t p0 = (static_cast<uint32_t>(1) << 24) | (static_cast<uint32_t>(2) << 16) |
                        (static_cast<uint32_t>(3) << 8) | static_cast<uint32_t>(4);
    const uint32_t p1 = (static_cast<uint32_t>(5) << 24) | (static_cast<uint32_t>(6) << 16) |
                        (static_cast<uint32_t>(7) << 8) | static_cast<uint32_t>(8);

    EXPECT_EQ(buf.getPixel(0, 0), p0);
    EXPECT_EQ(buf.getPixel(1, 0), p1);

    // areas outside source should remain zero (transparent)
    EXPECT_EQ(buf.getPixel(2, 0), 0u);
    EXPECT_EQ(buf.getPixel(3, 0), 0u);
    EXPECT_EQ(buf.getPixel(0, 1), 0u);
}
