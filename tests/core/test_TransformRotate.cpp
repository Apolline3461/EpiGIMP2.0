//
// Created by apolline on 12/03/2026.
//

// tests/core/test_TransformRotate.cpp
#include <gtest/gtest.h>

#include "core/Transform.hpp"
#include "core/ImageBuffer.hpp"
#include "common/Colors.hpp"

namespace {

// Helpers RGBA si tu veux des couleurs lisibles.
static inline uint32_t RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF) {
    return (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | uint32_t(a);
}

static void fill(ImageBuffer& img, uint32_t c) {
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            img.setPixel(x, y, c);
}

} // namespace

TEST(Transform_Rotate, KeepsDimensions)
{
    ImageBuffer src(7, 5);
    fill(src, RGBA(1,2,3));

    auto out = core::rotate(src, 17.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->width(), 7);
    EXPECT_EQ(out->height(), 5);
}

TEST(Transform_Rotate, ZeroDegrees_IsIdentity)
{
    ImageBuffer src(5, 3);
    // pattern unique par pixel
    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x)
            src.setPixel(x, y, RGBA(uint8_t(10 + x), uint8_t(20 + y), uint8_t(30 + x + y)));

    auto out = core::rotate(src, 0.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);

    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x)
            EXPECT_EQ(out->getPixel(x, y), src.getPixel(x, y));
}

TEST(Transform_Rotate, FullTurn360_IsIdentity)
{
    ImageBuffer src(5, 5);
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            src.setPixel(x, y, RGBA(uint8_t(x*20), uint8_t(y*20), uint8_t(100 + x + y)));

    auto out = core::rotate(src, 360.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);

    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            EXPECT_EQ(out->getPixel(x, y), src.getPixel(x, y));
}

TEST(Transform_Rotate, CenterPixel_StaysInPlace_ForOddSize)
{
    ImageBuffer src(5, 5);
    fill(src, common::colors::Transparent);

    const uint32_t centerColor = RGBA(200, 10, 10);
    src.setPixel(2, 2, centerColor);

    // peu importe l'angle, le centre exact (2,2) doit rester le centre (2,2)
    auto out = core::rotate(src, 37.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->getPixel(2, 2), centerColor);

    out = core::rotate(src, 90.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->getPixel(2, 2), centerColor);
}

TEST(Transform_Rotate, Rotate90_CW_MatchesExpected_For3x3)
{
    // On construit un 3x3 avec valeurs uniques :
    // a b c
    // d e f
    // g h i
    //
    // Rotation horaire 90° attendue :
    // g d a
    // h e b
    // i f c
    //
    // Avec ton mapping inverse et "degrees > 0" => rotation visuelle horaire.
    ImageBuffer src(3, 3);
    const uint32_t a = RGBA(1,0,0), b = RGBA(2,0,0), c = RGBA(3,0,0);
    const uint32_t d = RGBA(4,0,0), e = RGBA(5,0,0), f = RGBA(6,0,0);
    const uint32_t g = RGBA(7,0,0), h = RGBA(8,0,0), i = RGBA(9,0,0);

    src.setPixel(0,0,a); src.setPixel(1,0,b); src.setPixel(2,0,c);
    src.setPixel(0,1,d); src.setPixel(1,1,e); src.setPixel(2,1,f);
    src.setPixel(0,2,g); src.setPixel(1,2,h); src.setPixel(2,2,i);

    auto out = core::rotate(src, 90.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);

    EXPECT_EQ(out->getPixel(0,0), g);
    EXPECT_EQ(out->getPixel(1,0), d);
    EXPECT_EQ(out->getPixel(2,0), a);

    EXPECT_EQ(out->getPixel(0,1), h);
    EXPECT_EQ(out->getPixel(1,1), e);
    EXPECT_EQ(out->getPixel(2,1), b);

    EXPECT_EQ(out->getPixel(0,2), i);
    EXPECT_EQ(out->getPixel(1,2), f);
    EXPECT_EQ(out->getPixel(2,2), c);
}

TEST(Transform_Rotate, Rotate180_MatchesExpected_For3x3)
{
    // 180° :
    // i h g
    // f e d
    // c b a
    ImageBuffer src(3, 3);
    const uint32_t a = RGBA(1,0,0), b = RGBA(2,0,0), c = RGBA(3,0,0);
    const uint32_t d = RGBA(4,0,0), e = RGBA(5,0,0), f = RGBA(6,0,0);
    const uint32_t g = RGBA(7,0,0), h = RGBA(8,0,0), i = RGBA(9,0,0);

    src.setPixel(0,0,a); src.setPixel(1,0,b); src.setPixel(2,0,c);
    src.setPixel(0,1,d); src.setPixel(1,1,e); src.setPixel(2,1,f);
    src.setPixel(0,2,g); src.setPixel(1,2,h); src.setPixel(2,2,i);

    auto out = core::rotate(src, 180.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);

    EXPECT_EQ(out->getPixel(0,0), i);
    EXPECT_EQ(out->getPixel(1,0), h);
    EXPECT_EQ(out->getPixel(2,0), g);

    EXPECT_EQ(out->getPixel(0,1), f);
    EXPECT_EQ(out->getPixel(1,1), e);
    EXPECT_EQ(out->getPixel(2,1), d);

    EXPECT_EQ(out->getPixel(0,2), c);
    EXPECT_EQ(out->getPixel(1,2), b);
    EXPECT_EQ(out->getPixel(2,2), a);
}

TEST(Transform_Rotate, Rotate270_CW_MatchesExpected_For3x3)
{
    // 270° horaire == 90° anti-horaire
    // attendu :
    // c f i
    // b e h
    // a d g
    ImageBuffer src(3, 3);
    const uint32_t a = RGBA(1,0,0), b = RGBA(2,0,0), c = RGBA(3,0,0);
    const uint32_t d = RGBA(4,0,0), e = RGBA(5,0,0), f = RGBA(6,0,0);
    const uint32_t g = RGBA(7,0,0), h = RGBA(8,0,0), i = RGBA(9,0,0);

    src.setPixel(0,0,a); src.setPixel(1,0,b); src.setPixel(2,0,c);
    src.setPixel(0,1,d); src.setPixel(1,1,e); src.setPixel(2,1,f);
    src.setPixel(0,2,g); src.setPixel(1,2,h); src.setPixel(2,2,i);

    auto out = core::rotate(src, 270.f, common::colors::Transparent);
    ASSERT_NE(out, nullptr);

    EXPECT_EQ(out->getPixel(0,0), c);
    EXPECT_EQ(out->getPixel(1,0), f);
    EXPECT_EQ(out->getPixel(2,0), i);

    EXPECT_EQ(out->getPixel(0,1), b);
    EXPECT_EQ(out->getPixel(1,1), e);
    EXPECT_EQ(out->getPixel(2,1), h);

    EXPECT_EQ(out->getPixel(0,2), a);
    EXPECT_EQ(out->getPixel(1,2), d);
    EXPECT_EQ(out->getPixel(2,2), g);
}

TEST(Transform_Rotate, UsesBackgroundColor_WhenSourceOutside)
{
    ImageBuffer src(5, 5);
    const uint32_t fillColor = RGBA(1, 2, 3, 255);
    fill(src, fillColor);

    const uint32_t bg = RGBA(9, 9, 9, 255);
    auto out = core::rotate(src, 45.f, bg);
    ASSERT_NE(out, nullptr);

    bool foundBg = false;
    bool foundFill = false;

    for (int y = 0; y < out->height(); ++y)
    {
        for (int x = 0; x < out->width(); ++x)
        {
            const auto px = out->getPixel(x, y);
            if (px == bg) foundBg = true;
            if (px == fillColor) foundFill = true;
        }
    }

    EXPECT_TRUE(foundBg) << "Expected at least one pixel to be filled with bgColor";
    EXPECT_TRUE(foundFill) << "Expected at least one pixel to come from source";
}