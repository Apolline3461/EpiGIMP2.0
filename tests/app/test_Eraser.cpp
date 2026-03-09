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

    svc->beginStroke(p, common::Point{10, 10});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->getPixel(10, 10), common::colors::Black);

    // Erase
    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.color = common::colors::Black; // ignored by StrokeCommand (eraser forces transparent)

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
    e.color = common::colors::Black;

    svc->beginStroke(e, common::Point{5, 5});
    svc->endStroke();

    svc->beginStroke(e, common::Point{15, 15});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());

    EXPECT_EQ(layer->image()->getPixel(5, 5), common::colors::Transparent);
    EXPECT_EQ(layer->image()->getPixel(15, 15), common::colors::Black); // unchanged (not selected)
}