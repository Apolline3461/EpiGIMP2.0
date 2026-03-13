//
// test_DeleteSelection.cpp
//

#include "app/AppService.hpp"
#include "AppServiceUtilsForTest.hpp"
#include "common/Colors.hpp"
#include "common/Geometry.hpp"
#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

#include <gtest/gtest.h>

TEST(AppService_DeleteSelection, DeletesPixelsWithinMaskOnActiveLayer)
{
    const auto app = makeApp();
    app->newDocument({10, 10}, 72.f);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = common::colors::Transparent;
    app->addLayer(spec);  // layer idx 1

    app->setActiveLayer(1);

    auto layer = app->document().layerAt(1);
    ASSERT_NE(layer, nullptr);
    ASSERT_NE(layer->image(), nullptr);

    // fill layer with opaque color
    for (int y = 0; y < layer->image()->height(); ++y)
        for (int x = 0; x < layer->image()->width(); ++x)
            layer->image()->setPixel(x, y, 0x112233FFu);

    // set selection rect covering (1,1)-(3,3)
    app->setSelectionRect({1, 1, 3, 3});

    app->deleteSelection();

    // pixels inside selection should be transparent
    for (int gy = 0; gy < app->document().height(); ++gy)
    {
        for (int gx = 0; gx < app->document().width(); ++gx)
        {
            const int lx = gx - layer->offsetX();
            const int ly = gy - layer->offsetY();
            const bool inside = (gx >= 1 && gx < 1 + 3 && gy >= 1 && gy < 1 + 3);
            if (lx < 0 || ly < 0 || lx >= layer->image()->width() || ly >= layer->image()->height())
                continue;
            const uint32_t px = layer->image()->getPixel(lx, ly);
            if (inside)
                EXPECT_EQ(px, common::colors::Transparent);
            else
                EXPECT_EQ(px, 0x112233FFu);
        }
    }

    // selection should be cleared
    EXPECT_FALSE(app->document().selection().hasMask());
}

TEST(AppService_DeleteSelection, ThrowsWhenLayerLocked)
{
    const auto app = makeApp();
    app->newDocument({8, 8}, 72.f);

    app::LayerSpec spec;
    spec.name = "L1";
    app->addLayer(spec);
    app->setActiveLayer(1);
    app->setLayerLocked(1, true);

    app->setSelectionRect({0, 0, 2, 2});
    EXPECT_THROW(app->deleteSelection(), std::runtime_error);
}

TEST(AppService_DeleteSelection, NoOpOnBackground)
{
    const auto app = makeApp();
    app->newDocument({6, 6}, 72.f);

    auto bg = app->document().layerAt(0);
    ASSERT_NE(bg, nullptr);
    ASSERT_NE(bg->image(), nullptr);
    // remember a pixel
    const uint32_t before = bg->image()->getPixel(0, 0);

    app->setSelectionRect({0, 0, 2, 2});
    // active layer is 0 by default
    EXPECT_NO_THROW(app->deleteSelection());
    EXPECT_EQ(bg->image()->getPixel(0, 0), before);
}
