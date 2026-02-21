//
// Created by apolline on 20/02/2026.
//
#include <gtest/gtest.h>
#include "ui/PanClamp.hpp"

TEST(PanClamp, CentersImageWhenSmallerThanView)
{
    // image 100x100, scale 1 -> 100x100 sur écran
    // view 500x400 -> centre = (200,150)
    auto r = computeClampedPan(999, -999, 100, 100, 1.0, 500, 400, 32.0);
    EXPECT_DOUBLE_EQ(r.x, 200.0);
    EXPECT_DOUBLE_EQ(r.y, 150.0);
}

TEST(PanClamp, ClampsWhenImageBiggerThanView)
{
    // image 1000x800 scale 1 -> sw=1000, sh=800
    // view 500x400 margin=32
    // X in [-(1000-32), 500-32] = [-968, 468]
    // Y in [-(800-32), 400-32] = [-768, 368]
    auto r1 = computeClampedPan(9999, 9999, 1000, 800, 1.0, 500, 400, 32.0);
    EXPECT_DOUBLE_EQ(r1.x, 468.0);
    EXPECT_DOUBLE_EQ(r1.y, 368.0);

    auto r2 = computeClampedPan(-9999, -9999, 1000, 800, 1.0, 500, 400, 32.0);
    EXPECT_DOUBLE_EQ(r2.x, -968.0);
    EXPECT_DOUBLE_EQ(r2.y, -768.0);
}

TEST(PanClamp, DoesNotChangePanIfAlreadyInsideRange)
{
    auto r = computeClampedPan(0, -100, 1000, 800, 1.0, 500, 400, 32.0);
    // 0 est entre [-968,468], -100 entre [-768,368]
    EXPECT_DOUBLE_EQ(r.x, 0.0);
    EXPECT_DOUBLE_EQ(r.y, -100.0);
}

TEST(PanClamp, ZoomChangesClampRange)
{
    auto r = computeClampedPan(-5000, -5000, 1000, 800, 2.0, 500, 400, 32.0);
    // X range = [-(2000-32), 468] = [-1968, 468]
    // Y range = [-(1600-32), 368] = [-1568, 368]
    EXPECT_DOUBLE_EQ(r.x, -1968.0);
    EXPECT_DOUBLE_EQ(r.y, -1568.0);
}