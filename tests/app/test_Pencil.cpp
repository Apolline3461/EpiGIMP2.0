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

TEST(PencilTool, OpacityHalfOverWhiteGivesGray)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name="L1"; spec.width=20; spec.height=20; spec.color=common::colors::White;
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

    EXPECT_NEAR(A(px), 255, 1);
    EXPECT_NEAR(R(px), 128, 3);
    EXPECT_NEAR(G(px), 128, 3);
    EXPECT_NEAR(B(px), 128, 3);
}

TEST(PencilTool, OpacityHalfStillChangesPixels)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "Paint";
    spec.width = 20;
    spec.height = 20;
    spec.color = common::colors::White; // fond blanc dans le layer
    svc->addLayer(spec);

    const auto idx = svc->activeLayer();
    ASSERT_NE(idx, 0u);

    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = common::colors::Black; // 0x000000FF
    p.opacity = 0.5f;

    svc->beginStroke(p, {10, 10});
    svc->endStroke();

    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());

    const auto px = layer->image()->getPixel(10, 10);

    // Si opacity est tombée à 0 par erreur => pixel reste blanc (0xFFFFFFFF)
    EXPECT_NE(px, common::colors::White);
}
