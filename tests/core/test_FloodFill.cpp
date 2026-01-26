#include "core/FloodFill.hpp"
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
    floodFill(buf, 0, 0, A);

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
    floodFillWithinMask(buf, mask, 1, 1, A);

    EXPECT_EQ(buf.getPixel(1, 1), A);
    // neighbors should remain A (they were A originally)
    EXPECT_EQ(buf.getPixel(0, 1), A);
}
