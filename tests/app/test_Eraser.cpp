//
// Created by apolline on 09/03/2026.
//
#include <gtest/gtest.h>

#include "common/Colors.hpp"
#include "AppServiceUtilsForTest.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"
#include "app/ToolParams.hpp"

TEST(EraserTool, ErasesToTransparent)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 20;
    spec.height = 20;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    ASSERT_NE(idx, 0u);

    // Draw black dot
    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = common::colors::Black;
    p.opacity = 1.f;

    svc->beginStroke(p, common::Point{10, 10});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->offsetX(), 0);
    EXPECT_EQ(layer->offsetY(), 0);

    EXPECT_EQ(layer->image()->getPixel(10, 10), common::colors::Black);

    // Erase
    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.opacity = 1.f;

    svc->beginStroke(e, common::Point{10, 10});
    svc->endStroke();

    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->getPixel(10, 10), common::colors::Transparent);
}

TEST(EraserTool, RespectsSelection)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 20;
    spec.height = 20;
    spec.opacity = 1.f;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    ASSERT_NE(idx, 0u);

    // Fill two black pixels
    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = common::colors::Black;
    p.opacity = 1.f;

    svc->beginStroke(p, common::Point{5, 5});
    svc->endStroke();
    svc->beginStroke(p, common::Point{15, 15});
    svc->endStroke();

    // Select only around (5,5)
    svc->setSelectionRect(common::Rect{4, 4, 3, 3}); // includes 5,5, excludes 15,15

    // Erase both positions but selection should allow only first
    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.opacity = 1.f;

    svc->beginStroke(e, common::Point{5, 5});
    svc->endStroke();

    svc->beginStroke(e, common::Point{15, 15});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());

    EXPECT_EQ(layer->image()->getPixel(5, 5), common::colors::Transparent);
    EXPECT_EQ(layer->image()->getPixel(15, 15), common::colors::Black); // unchanged (not selected)
}

TEST(EraserTool, OpacityHalfDoesNotFullyErase)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name="L1";
    spec.width=20;
    spec.height=20;
    spec.color=common::colors::Black; // alpha=255
    svc->addLayer(spec);

    const auto idx = svc->activeLayer();

    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.opacity = 0.5f;

    svc->beginStroke(e, {10,10});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());

    const auto px = layer->image()->getPixel(10,10);

    EXPECT_NE(px, common::colors::Transparent);     // pas full erase
    EXPECT_NEAR(A(px), 128, 2);                     // alpha ~ 128
}

TEST(EraserTool, OpacityFullErasesToTransparent)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name="L1"; spec.width=20; spec.height=20; spec.color=common::colors::Black;
    svc->addLayer(spec);

    const auto idx = svc->activeLayer();

    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.opacity = 1.0f;

    svc->beginStroke(e, {10,10});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->getPixel(10,10), common::colors::Transparent);
}

TEST(EraserOpacity, ZeroDoesNotChangePixel)
{
    auto svc = makeApp();
    svc->newDocument({10, 10}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name="L1";
    spec.width=10;
    spec.height=10;
    spec.color=common::colors::Black; // alpha 255
    svc->addLayer(spec);

    auto layer = svc->document().layerAt(svc->activeLayer());
    ASSERT_TRUE(layer && layer->image());
    const uint32_t before = layer->image()->getPixel(2, 2);

    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.opacity = 0.f;

    svc->beginStroke(e, {2,2});
    svc->endStroke();

    layer = svc->document().layerAt(svc->activeLayer());
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->getPixel(2, 2), before);
}

TEST(Eraser_Opacity, OpacityHalf_OnHalfAlphaPixel_ReducesToQuarterAlpha)
{
    auto svc = makeApp();
    svc->newDocument({10, 10}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.width=10; spec.height=10;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    auto layer = svc->document().layerAt(svc->activeLayer());
    ASSERT_TRUE(layer && layer->image());

    // set pixel alpha = 128
    layer->image()->setPixel(2,2, RGBA(10,20,30,128));

    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.opacity = 0.5f;

    svc->beginStroke(e, {2,2});
    svc->endStroke();

    const auto px = layer->image()->getPixel(2,2);
    EXPECT_NEAR(A(px), 64, 3);
}

TEST(Eraser_Geometry, RespectsLayerOffset_DocToLocalMapping)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.width = 5;
    spec.height = 5;
    spec.color = common::colors::Black;
    spec.offsetX = 10;
    spec.offsetY = 10;
    svc->addLayer(spec);

    const auto idx = svc->activeLayer();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());

    // erase at doc (10,10) => local (0,0)
    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.opacity = 1.f;

    svc->beginStroke(e, {10,10});
    svc->endStroke();

    EXPECT_EQ(layer->image()->getPixel(0,0), common::colors::Transparent);
}