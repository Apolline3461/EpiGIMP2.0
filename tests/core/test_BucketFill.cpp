#include "core/BucketFill.hpp"
#include "core/ImageBuffer.hpp"

#include <gtest/gtest.h>

using namespace core;

TEST(FloodFillTest, SimpleFillRegion)
{
    ImageBuffer buf{5, 5};
    // fill all with color A
    const uint32_t A = 0x000000FFu;
    const uint32_t B = 0xFF0000FFu;
    buf.fill(A);

    // make a small region with color B at top-left 2x2
    buf.setPixel(0, 0, B);
    buf.setPixel(1, 0, B);
    buf.setPixel(0, 1, B);
    // run flood fill on (0,0) to change B -> A
    floodFill(buf, 0, 0, Color{A});

    EXPECT_EQ(buf.getPixel(0, 0), A);
    EXPECT_EQ(buf.getPixel(1, 0), A);
    EXPECT_EQ(buf.getPixel(0, 1), A);
    // other pixels remain A
    EXPECT_EQ(buf.getPixel(2, 2), A);
}

TEST(FloodFillTest, RespectMask)
{
    ImageBuffer buf{3, 3};
    const uint32_t A = 0x000000FFu;
    const uint32_t C = 0x00FF00FFu;
    buf.fill(A);

    ImageBuffer mask{3, 3};
    mask.fill(0x00000000u);
    // only center selected
    mask.setPixel(1, 1, 0x000000FFu);

    // change center to C
    buf.setPixel(1, 1, C);
    // attempt to flood fill C -> A within mask
    floodFillWithinMask(buf, mask, 1, 1, Color{A});

    EXPECT_EQ(buf.getPixel(1, 1), A);
    // neighbors should remain A (they were A originally)
    EXPECT_EQ(buf.getPixel(0, 1), A);
}

TEST(FloodFillTest, FloodFill_OutOfBounds_NoOp)
{
    ImageBuffer buf{3, 3};
    const uint32_t A = 0x11111111u;
    const uint32_t B = 0x22222222u;
    buf.fill(A);

    floodFill(buf, -1, 0, Color{B});
    floodFill(buf, 0, -1, Color{B});
    floodFill(buf, 3, 0, Color{B});
    floodFill(buf, 0, 3, Color{B});

    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            EXPECT_EQ(buf.getPixel(x, y), A);
}

TEST(FloodFillTest, FloodFill_TargetEqualsNewColor_NoOp)
{
    ImageBuffer buf{3, 3};
    const uint32_t A = 0xABCDEF01u;
    buf.fill(A);

    floodFill(buf, 1, 1, Color{A});

    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            EXPECT_EQ(buf.getPixel(x, y), A);
}

TEST(FloodFillTest, FloodFillWithinMask_ClickOutsideMask_NoOp)
{
    ImageBuffer buf{3, 3};
    const uint32_t A = 0x000000FFu;
    const uint32_t B = 0xFF0000FFu;
    buf.fill(A);
    buf.setPixel(1, 1, B);

    ImageBuffer mask{3, 3};
    mask.fill(0x00000000u); // rien sélectionné

    floodFillWithinMask(buf, mask, 1, 1, Color{A}); // start pixel not allowed

    EXPECT_EQ(buf.getPixel(1, 1), B); // unchanged
}

TEST(FloodFillTest, FloodFillWithinMask_FillsOnlyInsideMask)
{
    ImageBuffer buf{5, 5};
    const uint32_t A = 0x10101010u;
    const uint32_t B = 0x20202020u;
    const uint32_t C = 0x30303030u;

    buf.fill(A);

    // region B = 3x3 centered
    for (int y = 1; y <= 3; ++y)
        for (int x = 1; x <= 3; ++x)
            buf.setPixel(x, y, B);

    // mask = only a cross inside that region
    ImageBuffer mask{5, 5};
    mask.fill(0u);
    mask.setPixel(2, 1, 0x000000FFu);
    mask.setPixel(2, 2, 0x000000FFu);
    mask.setPixel(2, 3, 0x000000FFu);
    mask.setPixel(1, 2, 0x000000FFu);
    mask.setPixel(3, 2, 0x000000FFu);

    floodFillWithinMask(buf, mask, 2, 2, Color{C});

    // cross should be C
    EXPECT_EQ(buf.getPixel(2, 2), C);
    EXPECT_EQ(buf.getPixel(2, 1), C);
    EXPECT_EQ(buf.getPixel(2, 3), C);
    EXPECT_EQ(buf.getPixel(1, 2), C);
    EXPECT_EQ(buf.getPixel(3, 2), C);

    // but corners of the 3x3 region (still B) must remain B
    EXPECT_EQ(buf.getPixel(1, 1), B);
    EXPECT_EQ(buf.getPixel(3, 1), B);
    EXPECT_EQ(buf.getPixel(1, 3), B);
    EXPECT_EQ(buf.getPixel(3, 3), B);
}
