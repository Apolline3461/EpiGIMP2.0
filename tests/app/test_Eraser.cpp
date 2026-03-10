//
// Created by apolline on 09/03/2026.
//
#include <gtest/gtest.h>

#include "common/Colors.hpp"
#include "AppServiceUtilsForTest.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"
#include "app/ToolParams.hpp"

namespace
{
static inline uint8_t R(uint32_t rgba) { return static_cast<uint8_t>((rgba >> 24) & 0xFFu); }
static inline uint8_t G(uint32_t rgba) { return static_cast<uint8_t>((rgba >> 16) & 0xFFu); }
static inline uint8_t B(uint32_t rgba) { return static_cast<uint8_t>((rgba >> 8) & 0xFFu); }
static inline uint8_t A(uint32_t rgba) { return static_cast<uint8_t>(rgba & 0xFFu); }

static inline uint32_t RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | uint32_t(a);
}
}

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
    spec.color = common::colors::Transparent;
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

TEST(PencilTool, OpacityHalfBlendsOverWhiteToGray)
{
    auto svc = makeApp();
    svc->newDocument({10, 10}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 10;
    spec.height = 10;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    ASSERT_NE(idx, 0u);

    // First paint a white opaque pixel into the layer
    app::ToolParams w;
    w.tool = app::ToolKind::Pencil;
    w.size = 1;
    w.color = RGBA(255, 255, 255, 255);
    w.opacity = 1.f;
    svc->beginStroke(w, common::Point{2, 2});
    svc->endStroke();

    // Then paint black at 50% over it => should become gray, alpha stays 255
    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = common::colors::Black; // A=255
    p.opacity = 0.5f;
    svc->beginStroke(p, common::Point{2, 2});
    svc->endStroke();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    const uint32_t px = layer->image()->getPixel(2, 2);

    EXPECT_NEAR(R(px), 128, 1);
    EXPECT_NEAR(G(px), 128, 1);
    EXPECT_NEAR(B(px), 128, 1);
    EXPECT_EQ(A(px), 255);
}

TEST(EraserTool, OpacityHalfReducesAlpha)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name="L1"; spec.width=20; spec.height=20; spec.color=common::colors::Black;
    svc->addLayer(spec);

    const auto idx = svc->activeLayer();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    ASSERT_EQ(layer->image()->getPixel(10,10), common::colors::Black); // alpha=255

    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.opacity = 0.5f;

    svc->beginStroke(e, {10,10});
    svc->endStroke();

    layer = svc->document().layerAt(idx);
    const auto px = layer->image()->getPixel(10,10);
    EXPECT_NEAR(A(px), 128, 2);
}




TEST(PencilTool, OpacityZeroDoesNotChangePixel)
{
    auto svc = makeApp();
    svc->newDocument({10, 10}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 10;
    spec.height = 10;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    ASSERT_NE(idx, 0u);

    // Start with a known pixel
    app::ToolParams p1;
    p1.tool = app::ToolKind::Pencil;
    p1.size = 1;
    p1.color = common::colors::Black;
    p1.opacity = 1.f;
    svc->beginStroke(p1, common::Point{1, 1});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    const uint32_t before = layer->image()->getPixel(1, 1);

    // Paint with opacity 0 => must not change
    app::ToolParams p2;
    p2.tool = app::ToolKind::Pencil;
    p2.size = 1;
    p2.color = RGBA(255, 0, 0, 255);
    p2.opacity = 0.f;

    svc->beginStroke(p2, common::Point{1, 1});
    svc->endStroke();

    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    const uint32_t after = layer->image()->getPixel(1, 1);

    EXPECT_EQ(after, before);
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