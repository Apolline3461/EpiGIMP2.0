//
// Created by apolline on 22/02/2026.
//

#include <gtest/gtest.h>

#include <cstdint>

#include "app/AppService.hpp"
#include "AppServiceUtilsForTest.hpp"
#include "app/ToolParams.hpp"
#include "common/Colors.hpp"
#include "core/Layer.hpp"
#include "core/ImageBuffer.hpp"

// Helpers
static std::uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (std::uint32_t(r) << 24) | (std::uint32_t(g) << 16) | (std::uint32_t(b) << 8) |
           std::uint32_t(a);
}

static std::size_t countNonTransparent(const ImageBuffer& img)
{
    std::size_t n = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.getPixel(x, y) != common::colors::Transparent)
                ++n;
    return n;
}

static void expectAllTransparent(const ImageBuffer& img)
{
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            EXPECT_EQ(img.getPixel(x, y), common::colors::Transparent);
}

TEST(AppService_PencilSelection, StrokeOutsideSelectionDoesNothing)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "Paint";
    spec.width = 20;
    spec.height = 20;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    ASSERT_NE(idx, 0u);

    // sélection: 5x5 en haut à gauche (x:0-4, y:0-4)
    svc->setSelectionRect(common::Rect{0, 0, 5, 5});

    app::ToolParams params;
    params.tool = app::ToolKind::Pencil;
    params.size = 1;                     // important: ne pas "déborder" dans la sélection
    params.color = rgba(0, 0, 0, 255);    // noir opaque

    // Stroke hors sélection
    svc->beginStroke(params, common::Point{10, 10});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer);
    ASSERT_TRUE(layer->image());

    // Aucun pixel ne doit avoir changé (tout transparent)
    expectAllTransparent(*layer->image());
}

TEST(AppService_PencilSelection, StrokeInsideSelectionPaintsOnlyInside)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "Paint";
    spec.width = 20;
    spec.height = 20;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    ASSERT_NE(idx, 0u);

    // sélection: 5x5 en haut à gauche (x:0-4, y:0-4)
    const common::Rect sel{0, 0, 5, 5};
    svc->setSelectionRect(sel);

    app::ToolParams params;
    params.tool = app::ToolKind::Pencil;
    params.size = 3;                  // peut déborder, mais doit être CLIP par la sélection
    params.color = rgba(0, 0, 0, 255);

    // Stroke centré dans la sélection
    svc->beginStroke(params, common::Point{2, 2});
    svc->moveStroke(common::Point{4, 4});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer);
    ASSERT_TRUE(layer->image());
    const ImageBuffer& img = *layer->image();

    // Il doit y avoir au moins un pixel peint
    EXPECT_GT(countNonTransparent(img), 0u);

    // Et aucun pixel ne doit être peint en dehors de la sélection
    for (int y = 0; y < img.height(); ++y)
    {
        for (int x = 0; x < img.width(); ++x)
        {
            const bool insideSel = (x >= sel.x && y >= sel.y && x < sel.x + sel.w && y < sel.y + sel.h);

            if (!insideSel) {
                EXPECT_EQ(img.getPixel(x, y), common::colors::Transparent)
                << "Pixel painted outside selection at (" << x << "," << y << ")";
            }
        }
    }
}

TEST(AppService_PencilSelection, NoSelectionPaintsNormally)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "Paint";
    spec.width = 20;
    spec.height = 20;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    const std::size_t idx = svc->activeLayer();
    ASSERT_NE(idx, 0u);

    // pas de sélection
    svc->clearSelectionRect();

    app::ToolParams params;
    params.tool = app::ToolKind::Pencil;
    params.size = 1;
    params.color = rgba(0, 0, 0, 255);

    svc->beginStroke(params, common::Point{10, 10});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer);
    ASSERT_TRUE(layer->image());

    EXPECT_GT(countNonTransparent(*layer->image()), 0u);
}