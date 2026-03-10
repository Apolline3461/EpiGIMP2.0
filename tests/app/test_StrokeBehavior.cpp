//
// Created by apolline on 10/03/2026.
//

#include <gtest/gtest.h>

#include "AppServiceUtilsForTest.hpp"
#include "app/ToolParams.hpp"
#include "common/Colors.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

TEST(StrokeBehavior_History, Stroke_DrawsPixels_AndIsUndoable)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 3}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = common::colors::Transparent;
    app->addLayer(spec);
    app->setActiveLayer(1);

    app::ToolParams tp{};
    tp.tool = app::ToolKind::Pencil;
    tp.color = RGBA(0x11, 0x22, 0x33, 0xFF); // == 0x112233FFu
    tp.opacity = 1.f;
    tp.size = 1;

    app->beginStroke(tp, common::Point{1, 1});
    app->moveStroke(common::Point{4, 1});
    app->endStroke();

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);

    // pixels (1..4,1) modifiés
    for (int x = 1; x <= 4; ++x)
        EXPECT_EQ(img->getPixel(x, 1), RGBA(0x11, 0x22, 0x33, 0xFF));

    // undo
    ASSERT_TRUE(app->canUndo());
    app->undo();
    for (int x = 1; x <= 4; ++x)
        EXPECT_EQ(img->getPixel(x, 1), common::colors::Transparent);

    ASSERT_TRUE(app->canRedo());
    app->redo();
    for (int x = 1; x <= 4; ++x)
        EXPECT_EQ(img->getPixel(x, 1), RGBA(0x11, 0x22, 0x33, 0xFF));
}

TEST(StrokeBehavior_Geometry, MultiPointStrokePaintsMultiplePixels)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 20;
    spec.height = 20;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = common::colors::Black;
    p.opacity = 1.f;

    svc->beginStroke(p, {2, 2});
    svc->moveStroke({10, 2});
    svc->endStroke();

    const auto idx = svc->activeLayer();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());

    // On s'attend à ce qu'il y ait plusieurs pixels noirs sur la ligne y=2
    int count = 0;
    for (int x = 0; x < 20; ++x)
        if (layer->image()->getPixel(x, 2) == common::colors::Black)
            ++count;

    EXPECT_GT(count, 2); // pas juste un point
}

TEST(StrokeBehavior_Geometry, SinglePointStrokePaintsOneStamp)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.color = common::colors::Transparent;
    app->addLayer(spec);
    app->setActiveLayer(1);

    app::ToolParams tp{};
    tp.tool = app::ToolKind::Pencil;
    tp.size = 1;
    tp.color = common::colors::Black;
    tp.opacity = 1.f;

    app->beginStroke(tp, {3, 3});
    app->endStroke();

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->getPixel(3, 3), common::colors::Black);
}

TEST(StrokeBehavior_Selection, BrushSizeClipsToSelection)
{
    auto svc = makeApp();
    svc->newDocument({20, 20}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 20;
    spec.height = 20;
    spec.color = common::colors::Transparent;
    svc->addLayer(spec);

    // selection 5x5 (0..4)
    common::Rect sel{0, 0, 5, 5};
    svc->setSelectionRect(sel);

    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 7; // volontairement grand (déborderait sans clip)
    p.color = common::colors::Black;
    p.opacity = 1.f;

    svc->beginStroke(p, {2, 2});
    svc->endStroke();

    const auto idx = svc->activeLayer();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    const auto& img = *layer->image();

    for (int y = 0; y < img.height(); ++y)
    {
        for (int x = 0; x < img.width(); ++x)
        {
            const bool inside = (x >= sel.x && y >= sel.y && x < sel.x + sel.w && y < sel.y + sel.h);
            if (!inside)
            {
                EXPECT_EQ(img.getPixel(x, y), common::colors::Transparent)
                    << "painted outside selection at (" << x << "," << y << ")";
            }
        }
    }
}

TEST(StrokeBehavior_Selection, EmptySelectionDoesNotClip)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.color = common::colors::Transparent;
    app->addLayer(spec);
    app->setActiveLayer(1);

    // sélection vide (selon ton impl, ça devrait clear)
    app->setSelectionRect(common::Rect{0, 0, 0, 0});

    app::ToolParams tp{};
    tp.tool = app::ToolKind::Pencil;
    tp.size = 1;
    tp.color = common::colors::Black;
    tp.opacity = 1.f;

    app->beginStroke(tp, {8, 8});
    app->endStroke();

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->getPixel(8, 8), common::colors::Black);
}

TEST(StrokeBehavior_Offsets, DrawingUsesDocumentCoordinatesWithLayerOffset)
{
    auto svc = makeApp();
    svc->newDocument({30, 30}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 10;
    spec.height = 10;
    spec.color = common::colors::Transparent;
    spec.offsetX = 7;
    spec.offsetY = 9;
    svc->addLayer(spec);

    const auto idx = svc->activeLayer();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    ASSERT_EQ(layer->offsetX(), 7);
    ASSERT_EQ(layer->offsetY(), 9);

    // On dessine en coords document : (7,9) doit aller sur pixel local (0,0) dans l'image du layer
    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 1;
    p.color = common::colors::Black;
    p.opacity = 1.f;

    svc->beginStroke(p, {7, 9});
    svc->endStroke();

    layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->getPixel(0, 0), common::colors::Black);
}

TEST(StrokeBehavior_Bounds, StampingOutsideLayerDoesNotCrashAndDoesNotPaint)
{
    auto svc = makeApp();
    svc->newDocument({30, 30}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name = "L1";
    spec.width = 5;
    spec.height = 5;
    spec.color = common::colors::Transparent;
    spec.offsetX = 10;
    spec.offsetY = 10;
    svc->addLayer(spec);

    app::ToolParams p;
    p.tool = app::ToolKind::Pencil;
    p.size = 5;
    p.color = common::colors::Black;
    p.opacity = 1.f;

    // point complètement hors du layer (doc coords)
    svc->beginStroke(p, {0, 0});
    svc->endStroke();

    const auto idx = svc->activeLayer();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    // tout doit rester transparent
    for (int y = 0; y < layer->image()->height(); ++y)
        for (int x = 0; x < layer->image()->width(); ++x)
            EXPECT_EQ(layer->image()->getPixel(x, y), common::colors::Transparent);
}

TEST(StrokeBehavior_Eraser, ErasingTransparentStaysTransparent)
{
    auto svc = makeApp();
    svc->newDocument({10, 10}, 72.f, common::colors::White);

    app::LayerSpec spec;
    spec.name="L1";
    spec.width=10;
    spec.height=10;
    spec.color=common::colors::Transparent;
    svc->addLayer(spec);

    app::ToolParams e;
    e.tool = app::ToolKind::Eraser;
    e.size = 1;
    e.opacity = 1.f;

    svc->beginStroke(e, {2,2});
    svc->endStroke();

    const auto idx = svc->activeLayer();
    auto layer = svc->document().layerAt(idx);
    ASSERT_TRUE(layer && layer->image());
    EXPECT_EQ(layer->image()->getPixel(2, 2), common::colors::Transparent);
}

TEST(StrokeBehavior_API, BeginStroke_OnLockedLayer_Throws)
{
    const auto app = makeApp();
    app->newDocument(app::Size{3, 3}, 72.f);

    app::LayerSpec spec{};
    spec.locked = true;
    app->addLayer(spec);

    app::ToolParams tp{};
    EXPECT_THROW(app->beginStroke(tp, common::Point{0, 0}), std::runtime_error);
}

TEST(StrokeBehavior_API, BeginStroke_Twice_ThrowsLogicError)
{
    const auto app = makeApp();
    app->newDocument(app::Size{5, 5}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = common::colors::Transparent;
    app->addLayer(spec);
    app->setActiveLayer(1);

    app::ToolParams tp{};
    tp.tool = app::ToolKind::Pencil;
    tp.color = common::colors::Black;

    app->beginStroke(tp, {1, 1});
    EXPECT_THROW(app->beginStroke(tp, {2, 2}), std::logic_error);

    // cleanup
    app->endStroke();
}

TEST(StrokeBehavior_API, EndStroke_WithoutBegin_NoOp)
{
    const auto app = makeApp();
    app->newDocument(app::Size{3, 3}, 72.f);

    EXPECT_FALSE(app->canUndo());
    EXPECT_NO_THROW(app->endStroke());
    EXPECT_FALSE(app->canUndo());
}

TEST(StrokeBehavior_API, MoveStroke_WithoutBegin_NoOp)
{
    const auto app = makeApp();
    app->newDocument(app::Size{3, 3}, 72.f);

    EXPECT_NO_THROW(app->moveStroke(common::Point{1, 1}));
    EXPECT_FALSE(app->canUndo());
}

