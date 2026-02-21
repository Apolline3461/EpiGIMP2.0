//
// Created by apolline on 21/02/2026.
//

#include <gtest/gtest.h>

#include "app/AppService.hpp"
#include "AppServiceUtilsForTest.hpp"
#include "common/Colors.hpp"
#include "core/Layer.hpp"
#include "app/commands/CommandUtils.hpp"

TEST(AppService_MoveLayer, MoveUndoRedo)
{
    auto svc = makeApp();
    svc->newDocument({100, 100}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 10;
    spec.height = 10;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    // active layer should be last (L1)
    const std::size_t idx = svc->activeLayer();
    ASSERT_NE(idx, 0u);

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer);
    EXPECT_EQ(layer->offsetX(), 0);
    EXPECT_EQ(layer->offsetY(), 0);

    svc->moveLayer(idx, 12, 34);

    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer);
    EXPECT_EQ(layer->offsetX(), 12);
    EXPECT_EQ(layer->offsetY(), 34);

    ASSERT_TRUE(svc->canUndo());
    svc->undo();

    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer);
    EXPECT_EQ(layer->offsetX(), 0);
    EXPECT_EQ(layer->offsetY(), 0);

    ASSERT_TRUE(svc->canRedo());
    svc->redo();

    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer);
    EXPECT_EQ(layer->offsetX(), 12);
    EXPECT_EQ(layer->offsetY(), 34);
}

TEST(AppService_MoveLayer, LockedLayerThrows)
{
    auto svc = makeApp();
    svc->newDocument({100, 100}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 10;
    spec.height = 10;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer);

    svc->setLayerLocked(idx, true);
    EXPECT_THROW(svc->moveLayer(idx, 5, 5), std::runtime_error);
}

TEST(AppService_MoveLayer, BackgroundDoesNotMove)
{
    auto svc = makeApp();
    svc->newDocument({100, 100}, 72.f, common::colors::White);

    auto bg = svc->document().layerAt(0);
    ASSERT_TRUE(bg);
    const int beforeX = bg->offsetX();
    const int beforeY = bg->offsetY();

    svc->moveLayer(0, 50, 60);

    bg = svc->document().layerAt(0);
    ASSERT_TRUE(bg);
    EXPECT_EQ(bg->offsetX(), beforeX);
    EXPECT_EQ(bg->offsetY(), beforeY);

    EXPECT_FALSE(svc->canUndo());
    EXPECT_FALSE(svc->canRedo());
}

TEST(AppService_MoveLayer, SameOffsetDoesNotPushHistory)
{
    app::AppService svc(nullptr);
    svc.newDocument({100, 100}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 10;
    spec.height = 10;
    spec.color = common::colors::Transparent;
    spec.offsetX = 0;
    spec.offsetY = 0;

    svc.addLayer(spec);

    const std::size_t idx = svc.activeLayer();
    ASSERT_NE(idx, 0u);

    auto layer = svc.document().layerAt(idx);
    ASSERT_TRUE(layer);
    const std::uint64_t layerId = layer->id();
    EXPECT_EQ(layer->offsetX(), 0);
    EXPECT_EQ(layer->offsetY(), 0);

    // no-op
    svc.moveLayer(idx, 0, 0);

    // Undo ONCE:
    // If moveLayer pushed, the layer still exists (only position changes).
    // If moveLayer didn't push, undo removes the layer (undo addLayer).
    svc.undo();

    auto idxAfter = app::commands::findLayerIndexById(svc.document(), layerId);
    EXPECT_FALSE(idxAfter.has_value()); // layer should be gone if no command was pushed by moveLayer
}

TEST(AppService_MoveLayer, OutOfRangeThrows)
{
    auto svc = makeApp();
    svc->newDocument({10, 10}, 72.f, common::colors::White);

    const std::size_t badIdx = 999;
    EXPECT_THROW(svc->moveLayer(badIdx, 1, 1), std::out_of_range);
}

TEST(AppService_MoveLayer, DoesNotChangeActiveLayer)
{
    auto svc = makeApp();
    svc->newDocument({100, 100}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 10;
    spec.height = 10;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const auto idx = svc->activeLayer();
    svc->moveLayer(idx, 10, 10);
    EXPECT_EQ(svc->activeLayer(), idx);

    svc->undo();
    EXPECT_EQ(svc->activeLayer(), idx);

    svc->redo();
    EXPECT_EQ(svc->activeLayer(), idx);
}