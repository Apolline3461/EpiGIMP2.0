#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "core/BucketFill.hpp"
#include "core/ImageBuffer.hpp"

#include <gtest/gtest.h>

using namespace core;

static void measureFill(const ImageBuffer& base, int startX, int startY, uint32_t newColor,
                        const std::string& scenario, int runs = 3)
{
    using clk = std::chrono::steady_clock;
    std::chrono::duration<double, std::milli> total{0};

    for (int i = 0; i < runs; ++i)
    {
        ImageBuffer tmp = base;  // copy baseline
        auto t0 = clk::now();
        floodFill(tmp, startX, startY, newColor);
        auto t1 = clk::now();
        total += (t1 - t0);
    }

    double avg = total.count() / runs;
    std::cout << "[BENCH] " << scenario << " avg " << avg << " ms over " << runs << " runs\n";
}

static ImageBuffer makeCheckerboard(int w, int h, uint32_t a, uint32_t b)
{
    ImageBuffer buf{w, h};
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            if (((x + y) & 1) == 0)
                buf.setPixel(x, y, a);
            else
                buf.setPixel(x, y, b);
        }
    }
    return buf;
}

static ImageBuffer makeRandom(int w, int h, uint32_t a, uint32_t b, uint32_t seed = 12345)
{
    ImageBuffer buf{w, h};
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, 1);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            buf.setPixel(x, y, dist(rng) ? a : b);
    return buf;
}

TEST(FloodFillBenchmark, DISABLED_PerfVariousSizes)
{
    const std::vector<int> sizes = {256, 512};
    const uint32_t A = 0x000000FFu;  // target
    const uint32_t B = 0xFF0000FFu;  // other
    const uint32_t NEW = 0x00FF00FFu;

    for (int s : sizes)
    {
        // Solid image: entire image is target -> floodFill should touch all pixels
        ImageBuffer solid{s, s};
        solid.fill(A);
        measureFill(solid, s / 2, s / 2, NEW, "solid " + std::to_string(s));

        // Checkerboard: target pixels are non-contiguous -> small fills
        ImageBuffer checker = makeCheckerboard(s, s, A, B);
        measureFill(checker, 0, 0, NEW, "checkerboard " + std::to_string(s));

        // Random noise: many small regions
        ImageBuffer rnd = makeRandom(s, s, A, B, /*seed=*/s);
        measureFill(rnd, s / 3, s / 3, NEW, "random " + std::to_string(s));
    }

    SUCCEED();
}
