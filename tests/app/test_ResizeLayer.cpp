//
// Created by apolline on 21/02/2026.
//

#include <gtest/gtest.h>

#include "app/AppService.hpp"
#include "AppServiceUtilsForTest.hpp"
#include "common/Colors.hpp"
#include "app/commands/CommandUtils.hpp"
#include "core/Layer.hpp"
#include "core/ImageBuffer.hpp"

TEST(AppService_ResizeLayer, ResizeUndoRedo)
{
    auto svc = makeApp();
    svc->newDocument({100, 100}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 4;
    spec.height = 4;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->width(), 4);
    EXPECT_EQ(layer->image()->height(), 4);

    svc->resizeLayer(idx, 8, 2, false);

    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->width(), 8);
    EXPECT_EQ(layer->image()->height(), 2);

    ASSERT_TRUE(svc->canUndo());
    svc->undo();
    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->width(), 4);
    EXPECT_EQ(layer->image()->height(), 4);

    ASSERT_TRUE(svc->canRedo());
    svc->redo();
    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->width(), 8);
    EXPECT_EQ(layer->image()->height(), 2);
}

TEST(AppService_ResizeLayer, LockedLayerThrows)
{
    auto svc = makeApp();
    svc->newDocument({100, 100}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 4;
    spec.height = 4;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    svc->setLayerLocked(idx, true);

    EXPECT_THROW(svc->resizeLayer(idx, 8, 8, false), std::runtime_error);
}