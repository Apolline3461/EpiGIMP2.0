// tests/app/test_DuplicateLayer.cpp
#include "AppServiceUtilsForTest.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

#include <gtest/gtest.h>

TEST(AppService_DuplicateLayer, DuplicatesAndInsertsAtIdxPlusOne)
{
    auto svc = makeApp();
    svc->newDocument({8, 8}, 72.f);

    app::LayerSpec spec{};
    spec.name="L1"; spec.visible=true; spec.locked=false; spec.opacity=0.5f; spec.color=0u;
    spec.offsetX = 3;
    spec.offsetY = 4;
    svc->addLayer(spec);

    ASSERT_EQ(svc->document().layerCount(), 2u);

    // mark pixel
    auto orig = svc->document().layerAt(1);
    ASSERT_NE(orig, nullptr);
    orig->image()->setPixel(1,1, 0xFF010203u);

    svc->duplicateLayer(1);

    ASSERT_EQ(svc->document().layerCount(), 3u);

    // inserted just above -> idx 2
    auto dup = svc->document().layerAt(2);
    ASSERT_NE(dup, nullptr);

    EXPECT_EQ(dup->offsetX(), 3);
    EXPECT_EQ(dup->offsetY(), 4);
    EXPECT_FLOAT_EQ(dup->opacity(), 0.5f);
    EXPECT_TRUE(dup->visible());
    EXPECT_FALSE(dup->locked());

    ASSERT_NE(dup->image(), nullptr);
    EXPECT_EQ(dup->image()->getPixel(1,1), 0xFF010203u);

    // command sets activeLayer to insertAt
    EXPECT_EQ(svc->activeLayer(), 2u);
}

TEST(AppService_DuplicateLayer, DeepCopyNotShared)
{
    auto svc = makeApp();
    svc->newDocument({8, 8}, 72.f);

    app::LayerSpec spec{};
    spec.name="L1"; spec.visible=true; spec.locked=false; spec.opacity=1.f; spec.color=0u;
    svc->addLayer(spec);

    auto orig = svc->document().layerAt(1);
    ASSERT_NE(orig, nullptr);
    orig->image()->setPixel(0,0, 0xFF112233u);

    svc->duplicateLayer(1);

    auto dup = svc->document().layerAt(2);
    ASSERT_NE(dup, nullptr);
    EXPECT_EQ(dup->image()->getPixel(0,0), 0xFF112233u);

    // mutate original
    orig->image()->setPixel(0,0, 0xFFABCDEFu);
    EXPECT_EQ(dup->image()->getPixel(0,0), 0xFF112233u);
}

TEST(AppService_DuplicateLayer, LockedLayerCopyIsLocked)
{
    auto svc = makeApp();
    svc->newDocument({8, 8}, 72.f);

    app::LayerSpec spec{};
    spec.name="Locked"; spec.visible=true; spec.locked=false; spec.opacity=1.f; spec.color=0u;
    svc->addLayer(spec);
    svc->setLayerLocked(1, true);

    ASSERT_TRUE(svc->document().layerAt(1)->locked());

    svc->duplicateLayer(1);

    ASSERT_EQ(svc->document().layerCount(), 3u);
    auto dup = svc->document().layerAt(2);
    ASSERT_NE(dup, nullptr);
    EXPECT_TRUE(dup->locked());
}

TEST(AppService_DuplicateLayer, BackgroundCanBeDuplicated_CopyIsUnlocked)
{
    auto svc = makeApp();
    svc->newDocument({8, 8}, 72.f);

    ASSERT_EQ(svc->document().layerCount(), 1u);
    auto bg = svc->document().layerAt(0);
    ASSERT_NE(bg, nullptr);

    svc->duplicateLayer(0);

    ASSERT_EQ(svc->document().layerCount(), 2u);
    auto dup = svc->document().layerAt(1);
    ASSERT_NE(dup, nullptr);

    EXPECT_FALSE(dup->locked());
}

TEST(AppService_DuplicateLayer, UndoRedoRestoresLayerCount)
{
    auto svc = makeApp();
    svc->newDocument({8, 8}, 72.f);

    app::LayerSpec spec{};
    spec.name="L1"; spec.visible=true; spec.locked=false; spec.opacity=1.f; spec.color=0u;
    svc->addLayer(spec);

    ASSERT_EQ(svc->document().layerCount(), 2u);

    svc->duplicateLayer(1);
    ASSERT_EQ(svc->document().layerCount(), 3u);

    svc->undo();
    ASSERT_EQ(svc->document().layerCount(), 2u);

    svc->redo();
    ASSERT_EQ(svc->document().layerCount(), 3u);
}