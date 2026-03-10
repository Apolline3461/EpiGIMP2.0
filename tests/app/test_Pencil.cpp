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
    params.color = RGBA(0, 0, 0, 255);    // noir opaque

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
    params.color = RGBA(0, 0, 0, 255);

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
    params.color = RGBA(0, 0, 0, 255);

    svc->beginStroke(params, common::Point{10, 10});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer);
    ASSERT_TRUE(layer->image());

    EXPECT_GT(countNonTransparent(*layer->image()), 0u);
}

TEST(PencilTool, OpacityHalfOverTransparentSetsHalfAlpha)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name="L1"; spec.width=20; spec.height=20; spec.color=common::colors::Transparent;
    svc->addLayer(spec);

    const auto idx = svc->activeLayer();
    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = common::colors::Black;
    p.opacity = 0.5f;

    svc->beginStroke(p, {10,10});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    const auto px = layer->image()->getPixel(10,10);

    EXPECT_NEAR(A(px), 128, 2);
    EXPECT_NEAR(R(px), 0, 1);
    EXPECT_NEAR(G(px), 0, 1);
    EXPECT_NEAR(B(px), 0, 1);
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

TEST(PencilOpacity, DoesNotAccumulateWithinSameStrokeOnSamePixel)
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

    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = common::colors::Black; // RGBA, alpha=255
    p.opacity = 0.5f;

    // 1) Un seul point
    svc->beginStroke(p, {2, 2});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    const uint32_t once = layer->image()->getPixel(2, 2);

    // reset: undo pour revenir à transparent
    ASSERT_TRUE(svc->canUndo());
    svc->undo();

    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    ASSERT_EQ(layer->image()->getPixel(2, 2), common::colors::Transparent);

    // 2) Même pixel repassé plusieurs fois DANS LE MEME STROKE
    svc->beginStroke(p, {2, 2});
    svc->moveStroke({2, 2});
    svc->moveStroke({2, 2});
    svc->moveStroke({2, 2});
    svc->endStroke();

    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    const uint32_t many = layer->image()->getPixel(2, 2);

    // Si tu n'accumules pas, le résultat doit être IDENTIQUE
    EXPECT_EQ(many, once);

    // (Optionnel) petite assertion "sanity": alpha doit être ~ 128
    EXPECT_NEAR(int(A(many)), 128, 2);
}

TEST(Pencil_Opacity, OpacityOne_WritesExactColor)
{
    auto svc = makeApp();
    svc->newDocument({10, 10}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.width = 10; spec.height = 10;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = RGBA(10, 20, 30, 255);
    p.opacity = 1.f;

    svc->beginStroke(p, {5,5});
    svc->endStroke();

    auto layer = svc->document().layerAt(svc->activeLayer());
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->getPixel(5,5), RGBA(10,20,30,255));
}

TEST(Pencil_Opacity, OpacityHalf_WithSemiTransparentColor_BlendsCorrectly)
{
    auto svc = makeApp();
    svc->newDocument({10, 10}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.width = 10; spec.height = 10;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    // src alpha 128, opacity 0.5 => effective alpha ≈ 64
    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = RGBA(0, 0, 0, 128);
    p.opacity = 0.5f;

    svc->beginStroke(p, {2,2});
    svc->endStroke();

    auto layer = svc->document().layerAt(svc->activeLayer());
    ASSERT_TRUE(layer && layer->image());
    const auto px = layer->image()->getPixel(2,2);

    EXPECT_NEAR(A(px), 64, 3);
}