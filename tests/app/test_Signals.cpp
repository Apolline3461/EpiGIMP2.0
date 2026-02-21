//
// Created by apolline on 20/02/2026.
//

#include "app/AppService.hpp"
#include "AppServiceUtilsForTest.hpp"
#include "core/ImageBuffer.hpp"

#include <gtest/gtest.h>

TEST(AppService_Signals, documentChanged_EmittedOnNewDocument) {
    const auto app = makeApp();

    int hits = 0;
    app->documentChanged.connect([&]() { hits++; });

    app->newDocument(app::Size{10, 10}, 72.f);

    EXPECT_EQ(hits, 1);
}

TEST(AppService_Signals, AddLayer_EmitsDocumentChangedOnce)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app::LayerSpec spec{};
    app->addLayer(spec); // +1

    EXPECT_EQ(hits, 1);
    app->undo(); // +1
    app->redo(); // +1

    EXPECT_EQ(hits, 3);
}

TEST(AppService_Signals, SetLayerLocked_UndoRedo_EmitsDocumentChangedOnceEach)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->setLayerLocked(0, true); // +1
    EXPECT_EQ(hits, 1);

    app->undo(); // +1
    app->redo(); // +1
    EXPECT_EQ(hits, 3);
}

TEST(AppService_Signals, SetLayerVisible_UndoRedo_EmitsDocumentChangedOnceEach)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->setLayerVisible(0, false); // +1
    EXPECT_EQ(hits, 1);

    app->undo(); // +1
    app->redo(); // +1
    EXPECT_EQ(hits, 3);
}

TEST(AppService_Signals, SetLayerOpacity_UndoRedo_EmitsDocumentChangedOnceEach)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->setLayerOpacity(0, 0.25f); // +1
    EXPECT_EQ(hits, 1);

    app->undo(); // +1
    app->redo(); // +1
    EXPECT_EQ(hits, 3);
}

TEST(AppService_Signals, RemoveLayer_UndoRedo_EmitsDocumentChangedOnceEach)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    app->addLayer(spec);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->removeLayer(1); // +1
    EXPECT_EQ(hits, 1);

    app->undo(); // +1
    app->redo(); // +1
    EXPECT_EQ(hits, 3);
}

TEST(AppService_Signals, ReorderLayer_UndoRedo_EmitsDocumentChangedOnceEach)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    app->addLayer(spec);
    app->addLayer(spec);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->reorderLayer(2, 1); // +1
    EXPECT_EQ(hits, 1);

    app->undo(); // +1
    app->redo(); // +1
    EXPECT_EQ(hits, 3);
}

TEST(AppService_Signals, MergeLayerDown_UndoRedo_EmitsDocumentChangedOnceEach)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    app->addLayer(spec);
    app->addLayer(spec);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->mergeLayerDown(2); // +1
    EXPECT_EQ(hits, 1);

    app->undo(); // +1
    app->redo(); // +1
    EXPECT_EQ(hits, 3);
}

TEST(AppService_Signals, SetLayerLocked_NoChange_DoesNotEmit)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->setLayerLocked(0, false);

    EXPECT_EQ(hits, 0);
}

TEST(AppService_Signals, Selection_SetRect_EmitsDocumentChangedOnce)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->setSelectionRect(Selection::Rect{1, 1, 2, 2}); // +1
    EXPECT_EQ(hits, 1);
}

TEST(AppService_Signals, Selection_Clear_EmitsDocumentChangedOnce)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app->setSelectionRect(Selection::Rect{1, 1, 2, 2}); // on ignore ce hit
    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->clearSelectionRect(); // +1
    EXPECT_EQ(hits, 1);
}

TEST(AppService_Signals, Stroke_End_Undo_Redo_EmitsOnceEach)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 3}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    app->addLayer(spec);
    app->setActiveLayer(1);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app::ToolParams tp{};
    tp.color = 0xFF00FF00u;

    const int before = hits;

    app->beginStroke(tp, common::Point{1, 1});
    app->moveStroke(common::Point{4, 1});

    EXPECT_EQ(hits, before);

    app->endStroke();  // commit -> +1
    EXPECT_EQ(hits, before + 1);

    app->undo();       // +1
    app->redo();       // +1
    EXPECT_EQ(hits, before + 3);
}

TEST(AppService_Signals, BucketFill_NoSelection_EmitsOnce)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->bucketFill(common::Point{2, 2}, 0xFF112233u);
    EXPECT_EQ(hits, 1);
}

TEST(AppService_Signals, BucketFill_UndoRedo_EmitsOnceEach)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->bucketFill(common::Point{3, 3}, 0xFF00AA11u); // +1
    app->undo();                                      // +1
    app->redo();                                      // +1

    EXPECT_EQ(hits, 3);
}

TEST(AppService_Signals, BucketFill_WithSelection_ClickOutside_DoesNotEmit)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    // sélection au centre (2..3,2..3)
    app->setSelectionRect(Selection::Rect{2, 2, 2, 2});

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    // clic hors sélection => no-op
    app->bucketFill(common::Point{0, 0}, 0xFF0000FFu);

    EXPECT_EQ(hits, 0);
}

TEST(AppService_Signals, BucketFill_WithSelection_ClickInside_EmitsOnceEachWithUndoRedo)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    app->setSelectionRect(Selection::Rect{1, 1, 4, 4});

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->bucketFill(common::Point{2, 2}, 0xFF123456u); // +1
    app->undo();                                       // +1
    app->redo();                                       // +1

    EXPECT_EQ(hits, 3);
}

TEST(AppService_Signals, BucketFill_OutOfBounds_DoesNotEmit)
{
    const auto app = makeApp();
    app->newDocument(app::Size{4, 4}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    app->bucketFill(common::Point{-1, 0}, 0xFF010203u);
    app->bucketFill(common::Point{0, -1}, 0xFF010203u);
    app->bucketFill(common::Point{4, 0}, 0xFF010203u);
    app->bucketFill(common::Point{0, 4}, 0xFF010203u);

    EXPECT_EQ(hits, 0);
}

TEST(AppService_Signals, BucketFill_LockedLayer_ThrowsAndDoesNotEmit)
{
    const auto app = makeApp();
    app->newDocument(app::Size{4, 4}, 72.f);

    app::LayerSpec spec{};
    spec.locked = true;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    EXPECT_THROW(app->bucketFill(common::Point{1, 1}, 0xFF000000u), std::runtime_error);
    EXPECT_EQ(hits, 0);
}

TEST(AppService_Signals, ReplaceBackgroundWithImage_EmitsDocumentChangedOnce)
{
    const auto app = makeApp();
    app->newDocument(app::Size{3, 3}, 72.f, common::colors::Transparent);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    ImageBuffer src(3, 3);
    src.fill(0xFF445566u);

    app->replaceBackgroundWithImage(src, "opened");

    EXPECT_EQ(hits, 1);
}
