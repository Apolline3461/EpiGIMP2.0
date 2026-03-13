// Created by apolline on 12/03/2026.

#include <gtest/gtest.h>

#include "common/Colors.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Transform.hpp"

// helpers locaux (pas besoin de AppServiceUtilsForTest.hpp)
static inline uint32_t RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    return (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | uint32_t(a);
}

static inline void fill(ImageBuffer& img, uint32_t rgba)
{
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            img.setPixel(x, y, rgba);
}

TEST(Transform_Rotate, OutputIsLargeEnough_NoCrop_45deg)
{
    ImageBuffer src(7, 5);
    fill(src, common::colors::Transparent);
    src.setPixel(0, 0, RGBA(255, 0, 0, 255));
    src.setPixel(6, 4, RGBA(0, 255, 0, 255));

    auto out = core::rotate(src, 45.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);

    // No-crop => dimensions >= original (souvent strictement >)
    EXPECT_GE(out->width(), src.width());
    EXPECT_GE(out->height(), src.height());

    // Les pixels “marqueurs” doivent exister quelque part (pas perdus par crop)
    bool foundR = false, foundG = false;
    for (int y = 0; y < out->height(); ++y)
        for (int x = 0; x < out->width(); ++x)
        {
            const auto p = out->getPixel(x, y);
            if (p == RGBA(255, 0, 0, 255)) foundR = true;
            if (p == RGBA(0, 255, 0, 255)) foundG = true;
        }

    EXPECT_TRUE(foundR);
    EXPECT_TRUE(foundG);
}

TEST(Transform_Rotate, UsesBackgroundColor_WhenPaddingExists_45deg)
{
    ImageBuffer src(7, 5);
    fill(src, RGBA(10, 10, 10, 255)); // non-bg partout

    const uint32_t bg = RGBA(9, 9, 9, 255);
    auto out = core::rotate(src, 45.f, bg);
    ASSERT_NE(out, nullptr);

    // normalement, 45° => padding => taille augmente
    EXPECT_GE(out->width(), src.width());
    EXPECT_GE(out->height(), src.height());

    bool foundBg = false;
    for (int y = 0; y < out->height(); ++y)
        for (int x = 0; x < out->width(); ++x)
            if (out->getPixel(x, y) == bg)
                foundBg = true;

    EXPECT_TRUE(foundBg);
}

TEST(Transform_Rotate, CenterPixel_MapsToOutputCenter_ForOddSize_45deg)
{
    ImageBuffer src(5, 5);
    fill(src, common::colors::Transparent);

    const uint32_t center = RGBA(200, 10, 30, 255);
    src.setPixel(2, 2, center);

    auto out = core::rotate(src, 45.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);

    const int outCx = (out->width() - 1) / 2;
    const int outCy = (out->height() - 1) / 2;

    // Si ton rotate garde les centres cohérents, ce test tient.
    EXPECT_EQ(out->getPixel(outCx, outCy), center);
}

TEST(Transform_Rotate, FullTurn360_IsIdentity)
{
    ImageBuffer src(7, 5);
    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x)
            src.setPixel(x, y, RGBA(uint8_t(x * 30), uint8_t(y * 40), 7, 255));

    auto out = core::rotate(src, 360.f, RGBA(1, 2, 3, 255));
    ASSERT_NE(out, nullptr);

    // Pour que ça passe, ton rotate doit gérer 360 comme 0 (fast-path).
    ASSERT_EQ(out->width(), src.width());
    ASSERT_EQ(out->height(), src.height());

    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x)
            EXPECT_EQ(out->getPixel(x, y), src.getPixel(x, y));
}