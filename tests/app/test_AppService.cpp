//
// Created by apolline on 21/01/2026.
//

#include <memory>
#include <string>
#include <unordered_set>
#include <gtest/gtest.h>

#include "app/AppService.hpp"
#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"
#include "io/EpgTypes.hpp"
#include "io/IStorage.hpp"
#include "common/Geometry.hpp"
#include "common/Colors.hpp"

class SpyStorage final : public IStorage {
public:
    bool openCalled = false;
    bool saveCalled = false;
    bool exportCalled = false;

    std::string lastOpenPath;
    std::string lastSavePath;
    std::string lastExportPath;
    const Document* lastSavedDoc = nullptr;
    const Document* lastExportedDoc = nullptr;

    io::epg::OpenResult open(const std::string& path) override {
        openCalled = true;
        lastOpenPath = path;
        io::epg::OpenResult result;
        result.document = std::make_unique<Document>(1, 1, 72.f);
        return result;
    }

    void save(const Document& doc, const std::string& path) override {
        saveCalled = true;
        lastSavePath = path;
        lastSavedDoc = &doc;
    }

    void exportImage(const Document& doc, const std::string& path) override {
        exportCalled = true;
        lastExportPath = path;
        lastExportedDoc = &doc;
    }
};

static std::unique_ptr<app::AppService> makeApp(SpyStorage** outSpy = nullptr) {
    auto spy = std::make_unique<SpyStorage>();
    if (outSpy) *outSpy = spy.get();
    return std::make_unique<app::AppService>(std::move(spy));
}

static void addOneEditableLayer(app::AppService& svc, std::string name = "Layer 1")
{
    app::LayerSpec spec;
    spec.name = std::move(name);
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0x00000000u;
    svc.addLayer(spec);
}

TEST(AppService_State, documentReturnsConstRef) {
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    const Document* p1 = &app->document();
    const Document* p2 = &app->document();
    EXPECT_EQ(p1, p2);
}

TEST(AppService_State, newDocumentInitialState_Default) {
    const auto app = makeApp();

    app->newDocument(app::Size{.w=640, .h=480}, 72.F);

    const Document& doc = app->document();
    EXPECT_EQ(doc.width(), 640);
    EXPECT_EQ(doc.height(), 480);
    EXPECT_FLOAT_EQ(doc.dpi(), 72.F);

    EXPECT_EQ(app->activeLayer(), 0);

    EXPECT_FALSE(app->canUndo());
    EXPECT_FALSE(app->canRedo());
}

TEST(AppService_State, newDocumentInitialLayer_CountIsOne) {

    const auto app = makeApp();

    app->newDocument(app::Size{.w=100, .h=200}, 72.F);

    const Document& doc = app->document();
    EXPECT_EQ(doc.layerCount(), 1);

    auto bgLayer = doc.layerAt(0);
    ASSERT_NE(bgLayer, nullptr);

    EXPECT_TRUE(bgLayer->visible());
    EXPECT_FALSE(bgLayer->locked());
    EXPECT_FLOAT_EQ(bgLayer->opacity(), 1.f);

    ASSERT_NE(bgLayer->image(), nullptr);
    EXPECT_EQ(bgLayer->image()->width(), 100);
    EXPECT_EQ(bgLayer->image()->height(), 200);

    EXPECT_EQ(bgLayer->image()->getPixel(0,0), 0xFFFFFFFFU);
    EXPECT_EQ(bgLayer->image()->getPixel(99,199), 0xFFFFFFFFU);
}

TEST(AppService_State, ActiveLayerSet_ValidIndex) {
    const auto app = makeApp();
    app::LayerSpec spec{};

    app->newDocument(app::Size{10, 10}, 72.f);
    app->addLayer(spec);
    app->addLayer(spec);

    app->setActiveLayer(2);
    EXPECT_EQ(app->activeLayer(), 2);
}

TEST(AppService_State, ActiveLayerSet_OutOfRangeThrows) {
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    EXPECT_THROW(app->setActiveLayer(1), std::out_of_range);
}

TEST(AppService_State, LayerIds_AreUnique) {
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    app->addLayer(spec);
    app->addLayer(spec);
    app->addLayer(spec);

    const Document& doc = app->document();
    const int n = doc.layerCount();
    ASSERT_GT(n, 0);

    std::unordered_set<std::uint64_t> ids;
    ids.reserve(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i) {
        auto layer = doc.layerAt(i);
        ASSERT_NE(layer, nullptr);

        const auto id = layer->id();
        EXPECT_TRUE(ids.insert(id).second) << "Duplicate layer id detected: " << id;
    }
}

TEST(AppService_State, LayerIds_ResetOnNewDocument) {
    const auto app = makeApp();
    app::LayerSpec spec{};

    app->newDocument(app::Size{10, 10}, 72.f);
    app->addLayer(spec);
    auto id1 = app->document().layerAt(1)->id(); // 0 = bg, 1 = first added

    app->newDocument(app::Size{10, 10}, 72.f);
    app->addLayer(spec);
    auto id2 = app->document().layerAt(1)->id();

    EXPECT_EQ(id1, id2); // si tu resets nextLayerId_ à 1
}

TEST(AppService_State, Selection_SetRect_CreatesMaskInDocument)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    common::Rect r{2, 3, 4, 2};
    app->setSelectionRect(r);

    const auto& sel = app->document().selection();
    EXPECT_TRUE(sel.hasMask());
    ASSERT_NE(sel.mask(), nullptr);
    EXPECT_EQ(sel.mask()->width(), 10);
    EXPECT_EQ(sel.mask()->height(), 10);
}

TEST(AppService_State, Selection_Clear_RemovesMaskInDocument)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app->setSelectionRect(Selection::Rect{1, 1, 2, 2});
    ASSERT_TRUE(app->document().selection().hasMask());

    app->clearSelectionRect();
    EXPECT_FALSE(app->document().selection().hasMask());
    EXPECT_EQ(app->document().selection().mask(), nullptr);
}


TEST(AppService_Layers, RemoveLayer_WhenLocked_Throws) {
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app->document().layerAt(0)->setLocked(true);
    EXPECT_THROW(app->removeLayer(0), std::runtime_error);
    EXPECT_EQ(app->document().layerCount(), 1);
}

TEST(AppService_Layers, RemoveLayer_AfterUnlock_AllowsEmptyDocument) {
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app->setLayerLocked(0, false);
    app->removeLayer(0);

    EXPECT_EQ(app->document().layerCount(), 0);
}

TEST(AppService_IO, Open_CallsStorage) {
    SpyStorage* spy = nullptr;
    auto app = makeApp(&spy);

    app->open("foo.epg");

    ASSERT_NE(spy, nullptr);
    EXPECT_TRUE(spy->openCalled);
    EXPECT_EQ(spy->lastOpenPath, "foo.epg");
}
TEST(AppService_IO, Save_CallsStorage) {
    SpyStorage* spy = nullptr;
    auto app = makeApp(&spy);

    app->newDocument(app::Size{10, 10}, 72.f);
    const Document* current = &app->document();
    app->save("bar.epg");

    ASSERT_NE(spy, nullptr);
    EXPECT_TRUE(spy->saveCalled);
    EXPECT_EQ(spy->lastSavePath, "bar.epg");
    EXPECT_EQ(spy->lastSavedDoc, current);
}

TEST(AppService_IO, exportImage_CallsStorage) {
    SpyStorage* spy = nullptr;
    auto app = makeApp(&spy);

    app->newDocument(app::Size{10, 10}, 72.f);
    const Document* current = &app->document();

    app->exportImage("out.png");

    ASSERT_NE(spy, nullptr);
    EXPECT_TRUE(spy->exportCalled);
    EXPECT_EQ(spy->lastExportPath, "out.png");
    EXPECT_EQ(spy->lastExportedDoc, current);

}

TEST(AppService_UndoRedo, AddLayerCommand_UndoRedo_RestoresSameLayerId) {
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    ASSERT_EQ(app->document().layerCount(), 1);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.name = "L1";
    spec.color = 0xFF00FFFFu;

    app->addLayer(spec);
    ASSERT_EQ(app->document().layerCount(), 2);

    const auto idAdded = app->document().layerAt(1)->id();
    ASSERT_NE(idAdded, 0u);

    app->undo();
    EXPECT_EQ(app->document().layerCount(), 1);

    app->redo();
    ASSERT_EQ(app->document().layerCount(), 2);

    EXPECT_EQ(app->document().layerAt(1)->id(), idAdded);

    EXPECT_TRUE(app->canUndo());
    EXPECT_FALSE(app->canRedo());
}

TEST(AppService_UndoRedo, SetLayerLocked_UndoRedo_RestoresPreviousValue)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    ASSERT_FALSE(app->document().layerAt(0)->locked());

    app->setLayerLocked(0, true);
    EXPECT_TRUE(app->document().layerAt(0)->locked());
    EXPECT_TRUE(app->canUndo());
    EXPECT_FALSE(app->canRedo());

    app->undo();
    EXPECT_FALSE(app->document().layerAt(0)->locked());
    EXPECT_FALSE(app->canUndo());
    EXPECT_TRUE(app->canRedo());

    app->redo();
    EXPECT_TRUE(app->document().layerAt(0)->locked());
    EXPECT_TRUE(app->canUndo());
    EXPECT_FALSE(app->canRedo());
}

TEST(AppService_UndoRedo, SetLayerVisible_UndoRedo_RestoresPreviousValue)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    ASSERT_TRUE(app->document().layerAt(0)->visible());

    app->setLayerVisible(0, false);
    EXPECT_FALSE(app->document().layerAt(0)->visible());

    app->undo();
    EXPECT_TRUE(app->document().layerAt(0)->visible());

    app->redo();
    EXPECT_FALSE(app->document().layerAt(0)->visible());
}

TEST(AppService_UndoRedo, SetLayerOpacity_UndoRedo_RestoresPreviousValue)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    ASSERT_FLOAT_EQ(app->document().layerAt(0)->opacity(), 1.f);

    app->setLayerOpacity(0, 0.25f);
    EXPECT_FLOAT_EQ(app->document().layerAt(0)->opacity(), 0.25f);

    app->undo();
    EXPECT_FLOAT_EQ(app->document().layerAt(0)->opacity(), 1.f);

    app->redo();
    EXPECT_FLOAT_EQ(app->document().layerAt(0)->opacity(), 0.25f);
}

TEST(AppService_UndoRedo, RemoveLayer_UndoRedo_RestoresSameLayerId)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.name = "L1";
    app->addLayer(spec);

    ASSERT_EQ(app->document().layerCount(), 2);
    const auto removedId = app->document().layerAt(1)->id();

    app->removeLayer(1);
    EXPECT_EQ(app->document().layerCount(), 1);

    app->undo();
    ASSERT_EQ(app->document().layerCount(), 2);
    EXPECT_EQ(app->document().layerAt(1)->id(), removedId);

    app->redo();
    EXPECT_EQ(app->document().layerCount(), 1);
}

TEST(AppService_UndoRedo, ReorderLayer_UndoRedo_RestoresOrder)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.name = "L1";
    app->addLayer(spec);
    spec.name = "L2";
    app->addLayer(spec);

    // order: bg(0), L1(1), L2(2)
    const auto idL1 = app->document().layerAt(1)->id();
    const auto idL2 = app->document().layerAt(2)->id();

    app->reorderLayer(2, 1); // move L2 above L1
    EXPECT_EQ(app->document().layerAt(1)->id(), idL2);
    EXPECT_EQ(app->document().layerAt(2)->id(), idL1);

    app->undo();
    EXPECT_EQ(app->document().layerAt(1)->id(), idL1);
    EXPECT_EQ(app->document().layerAt(2)->id(), idL2);

    app->redo();
    EXPECT_EQ(app->document().layerAt(1)->id(), idL2);
    EXPECT_EQ(app->document().layerAt(2)->id(), idL1);
}

TEST(AppService_UndoRedo, MergeLayerDown_UndoRedo_RestoresLayerCount)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    app->addLayer(spec);
    app->addLayer(spec);

    ASSERT_EQ(app->document().layerCount(), 3);

    app->mergeLayerDown(2);
    EXPECT_EQ(app->document().layerCount(), 2);

    app->undo();
    EXPECT_EQ(app->document().layerCount(), 3);

    app->redo();
    EXPECT_EQ(app->document().layerCount(), 2);
}

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

TEST(AppService_Picking, pickColorAt_ReadsPixelFromActiveLayer)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFU;
    app->addLayer(spec);
    app->setActiveLayer(1);

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);
    img->setPixel(4, 5, 0xFF112233U);

    EXPECT_EQ(app->pickColorAt(common::Point{4, 5}), 0xFF112233U);
}

TEST(AppService_Picking, pickColorAt_OutOfBounds_ReturnsTransparent)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    app->addLayer(spec);
    app->setActiveLayer(1);

    EXPECT_EQ(app->pickColorAt(common::Point{-1, 0}), common::colors::Transparent);
    EXPECT_EQ(app->pickColorAt(common::Point{0, -1}), common::colors::Transparent);
    EXPECT_EQ(app->pickColorAt(common::Point{10, 0}), common::colors::Transparent);
    EXPECT_EQ(app->pickColorAt(common::Point{0, 10}), common::colors::Transparent);
}

TEST(AppService_Picking, pickColorAt_NoDocument_Throws)
{
    const auto app = makeApp();
    EXPECT_THROW(app->pickColorAt(common::Point{0, 0}), std::runtime_error);
}

TEST(AppService_Picking, pickColorAt_DoesNotEmitDocumentChanged)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    app->addLayer(spec);
    app->setActiveLayer(1);

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);
    img->setPixel(1, 1, 0xFF010203u);

    int hits = 0;
    app->documentChanged.connect([&]() { ++hits; });

    (void)app->pickColorAt(common::Point{1, 1});
    EXPECT_EQ(hits, 0);
}

TEST(AppService_Picking, pickColorAt_DoesNotAffectUndoRedo)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    app->addLayer(spec);
    app->setActiveLayer(1);

    const bool beforeUndo = app->canUndo();
    const bool beforeRedo = app->canRedo();

    (void)app->pickColorAt(common::Point{0, 0});

    EXPECT_EQ(app->canUndo(), beforeUndo);
    EXPECT_EQ(app->canRedo(), beforeRedo);
}

TEST(AppService_Picking, PickColorAt_RespectsLayerOffsetAndSize)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    // add 3x3 layer at offset (4,4), fill red
    app::LayerSpec spec{};
    spec.name = "tiny";
    spec.width = 3;
    spec.height = 3;
    spec.color = 0xFF0000FFu;
    spec.offsetX = 4;
    spec.offsetY = 4;

    app->addLayer(spec);
    app->setActiveLayer(1);

    // inside
    EXPECT_EQ(app->pickColorAt({4,4}), 0xFF0000FFu);

    // outside (but still within doc) => transparent
    EXPECT_EQ(app->pickColorAt({0,0}), common::colors::Transparent);
}


TEST(AppService_Stroke, Stroke_DrawsPixels_AndIsUndoable)
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
    tp.color = 0xFF112233u;

    app->beginStroke(tp, common::Point{1, 1});
    app->moveStroke(common::Point{4, 1});
    app->endStroke();

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);

    // pixels (1..4,1) modifiés
    for (int x = 1; x <= 4; ++x)
        EXPECT_EQ(img->getPixel(x, 1), 0xFF112233u);

    // undo
    ASSERT_TRUE(app->canUndo());
    app->undo();
    for (int x = 1; x <= 4; ++x)
        EXPECT_EQ(img->getPixel(x, 1), common::colors::Transparent);

    ASSERT_TRUE(app->canRedo());
    app->redo();
    for (int x = 1; x <= 4; ++x)
        EXPECT_EQ(img->getPixel(x, 1), 0xFF112233u);
}

TEST(AppService_Stroke, EndStroke_WithoutBegin_NoOp)
{
    const auto app = makeApp();
    app->newDocument(app::Size{3, 3}, 72.f);

    EXPECT_FALSE(app->canUndo());
    EXPECT_NO_THROW(app->endStroke());
    EXPECT_FALSE(app->canUndo());
}

TEST(AppService_Stroke, MoveStroke_WithoutBegin_NoOp)
{
    const auto app = makeApp();
    app->newDocument(app::Size{3, 3}, 72.f);

    EXPECT_NO_THROW(app->moveStroke(common::Point{1, 1}));
    EXPECT_FALSE(app->canUndo());
}

TEST(AppService_Stroke, BeginStroke_OnLockedLayer_Throws)
{
    const auto app = makeApp();
    app->newDocument(app::Size{3, 3}, 72.f);

    app::LayerSpec spec{};
    spec.locked = true;
    app->addLayer(spec);

    app::ToolParams tp{};
    EXPECT_THROW(app->beginStroke(tp, common::Point{0, 0}), std::runtime_error);
}

TEST(AppService_Stroke, DoesNotPaintOutsideLayer_AndPaintsLocalPixelInside)
{
    const auto app = makeApp();
    app->newDocument({10, 10}, 72.f, /*bg*/ 0x000000FFu);

    // Remplace layer actif par un layer 3x3 offset (4,4), pour contrôler exactement la zone.
    app::LayerSpec spec{};
    spec.name = "L1";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0x00000000u; // transparent
    app->addLayer(spec);

    ASSERT_EQ(app->document().layerCount(), 2u);

    // Force image buffer 3x3 + offset (4,4)
    auto layer = app->document().layerAt(1);
    ASSERT_NE(layer, nullptr);
    auto img = std::make_shared<ImageBuffer>(3, 3);
    img->fill(0x00000000u);
    layer->setImageBuffer(img);
    layer->setOffset(4, 4);
    app->setActiveLayer(1);

    // --- OUTSIDE : doc point (3,4) -> local (-1,0) => doit rien peindre
    {
        app::ToolParams params{};
        params.color = 0xFF00FFFFu;
        app->beginStroke(params, common::Point{3, 4});
        app->endStroke();

        // vérifie qu'aucun pixel du layer 3x3 n'a changé
        auto img = layer->image();
        ASSERT_NE(img, nullptr);
        for (int y = 0; y < img->height(); ++y)
            for (int x = 0; x < img->width(); ++x)
                EXPECT_EQ(img->getPixel(x, y), 0x00000000u);
    }

    // --- INSIDE : doc point (5,6) -> local (1,2) => doit peindre img(1,2)
    {
        app::ToolParams params{};
        params.color = 0x00FF00FFu; // vert opaque
        app->beginStroke(params, common::Point{5, 6});
        app->endStroke();

        auto img = layer->image();
        ASSERT_NE(img, nullptr);

        // pixel attendu peint
        EXPECT_EQ(img->getPixel(1, 2), 0x00FF00FFu);

        // un autre pixel reste transparent
        EXPECT_EQ(img->getPixel(0, 0), 0x00000000u);
    }
}

TEST(AppService_BucketFill, NoSelection_FillsAndUndoRedoWorks)
{
    const auto app = makeApp();
    app->newDocument(app::Size{5, 5}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = common::colors::Transparent;
    app->addLayer(spec);
    app->setActiveLayer(1);

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);

    img->setPixel(2, 2, 0xFF000000u); // zone source
    const uint32_t fill = 0xFFFF0000u;

    app->bucketFill(common::Point{2, 2}, fill);

    EXPECT_TRUE(app->canUndo());
    EXPECT_EQ(img->getPixel(2, 2), fill);

    app->undo();
    EXPECT_EQ(img->getPixel(2, 2), 0xFF000000u);

    app->redo();
    EXPECT_EQ(img->getPixel(2, 2), fill);
}

TEST(AppService_BucketFill, WithSelection_ClickOutside_IsNoOpAndNoHistory)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFF00FF00u; // vert
    app->addLayer(spec);
    app->setActiveLayer(1);

    // sélection au centre (2..3, 2..3)
    app->setSelectionRect(Selection::Rect{2, 2, 2, 2});

    const bool beforeUndo = app->canUndo();
    const bool beforeRedo = app->canRedo();

    // clic hors sélection
    app->bucketFill(common::Point{0, 0}, 0xFFFF0000u);

    // rien ne doit changer côté history
    EXPECT_EQ(app->canUndo(), beforeUndo);
    EXPECT_EQ(app->canRedo(), beforeRedo);
}

TEST(AppService_BucketFill, WithSelection_FillsOnlyInsideMask)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu; // blanc partout
    app->addLayer(spec);
    app->setActiveLayer(1);

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);

    // sélection (1..4,1..4) => 4x4
    app->setSelectionRect(Selection::Rect{1, 1, 4, 4});

    const uint32_t fill = 0xFF112233u;
    app->bucketFill(common::Point{2, 2}, fill);

    // dans la sélection
    EXPECT_EQ(img->getPixel(2, 2), fill);

    // hors sélection (0,0) doit rester blanc
    EXPECT_EQ(img->getPixel(0, 0), 0xFFFFFFFFu);
}

TEST(AppService_BucketFill, LockedLayer_Throws)
{
    const auto app = makeApp();
    app->newDocument(app::Size{4, 4}, 72.f);

    app::LayerSpec spec{};
    spec.locked = true;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    EXPECT_THROW(app->bucketFill(common::Point{1, 1}, 0xFF000000u), std::runtime_error);
}

TEST(AppService_BucketFill, OutOfBounds_IsNoOpAndNoHistory)
{
    const auto app = makeApp();
    app->newDocument(app::Size{4, 4}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    const bool beforeUndo = app->canUndo();
    app->bucketFill(common::Point{-1, 0}, 0xFF000000u);
    app->bucketFill(common::Point{0, -1}, 0xFF000000u);
    app->bucketFill(common::Point{4, 0}, 0xFF000000u);
    app->bucketFill(common::Point{0, 4}, 0xFF000000u);

    EXPECT_EQ(app->canUndo(), beforeUndo);
}

TEST(AppService_OpenImageFlow, ReplaceBackgroundWithImage_KeepsSingleLayerAndCopiesPixels)
{
    const auto app = makeApp();
    app->newDocument(app::Size{3, 2}, 72.f, common::colors::Transparent);

    ASSERT_EQ(app->document().layerCount(), 1);
    auto bg = app->document().layerAt(0);
    ASSERT_NE(bg, nullptr);
    ASSERT_NE(bg->image(), nullptr);

    ImageBuffer src(3, 2);
    src.fill(0u);
    src.setPixel(0, 0, 0xFF112233u);
    src.setPixel(2, 1, 0xFFABCDEFu);

    app->replaceBackgroundWithImage(src, "opened");

    ASSERT_EQ(app->document().layerCount(), 1);
    bg = app->document().layerAt(0);
    ASSERT_NE(bg, nullptr);
    ASSERT_NE(bg->image(), nullptr);

    EXPECT_EQ(bg->name(), "opened");
    EXPECT_EQ(bg->image()->getPixel(0, 0), 0xFF112233u);
    EXPECT_EQ(bg->image()->getPixel(2, 1), 0xFFABCDEFu);
    EXPECT_EQ(app->activeLayer(), 0u);
}

TEST(AppService_OpenImageFlow, ReplaceBackgroundWithImage_SmallerSource_LeavesRestTransparent)
{
    const auto app = makeApp();
    app->newDocument(app::Size{4, 4}, 72.f, common::colors::Transparent);

    // source 2x2 seulement
    ImageBuffer src(2, 2);
    src.fill(0u);
    src.setPixel(1, 1, 0xFF010203u);

    app->replaceBackgroundWithImage(src, "opened");

    auto bg = app->document().layerAt(0);
    ASSERT_NE(bg, nullptr);
    ASSERT_NE(bg->image(), nullptr);

    // pixel copié
    EXPECT_EQ(bg->image()->getPixel(1, 1), 0xFF010203u);

    // zone hors source -> transparent (car on remplit out->fill(0u))
    EXPECT_EQ(bg->image()->getPixel(3, 3), 0u);
}

TEST(AppService_OpenImageFlow, ReplaceBackgroundWithImage_ClearsUndoRedo)
{
    const auto app = makeApp();
    app->newDocument(app::Size{3, 3}, 72.f, common::colors::Transparent);

    app::LayerSpec spec{};
    spec.locked = false;
    app->addLayer(spec);
    ASSERT_TRUE(app->canUndo());

    ImageBuffer src(3, 3);
    src.fill(0xFF0000FFu);

    app->replaceBackgroundWithImage(src, "opened");

    EXPECT_FALSE(app->canUndo());
    EXPECT_FALSE(app->canRedo());
}

TEST(AppService_SetLayerName, ChangesNameAndUndoRedoWorks)
{
    app::AppService svc(nullptr);
    svc.newDocument({32, 32}, 72.f);

    // addLayer pushes a command -> canUndo() is expected to be true now
    addOneEditableLayer(svc, "Layer 1");
    ASSERT_EQ(svc.document().layerCount(), 2u);
    ASSERT_TRUE(svc.canUndo());

    // Rename (this should add ONE more command if implemented via apply)
    svc.setLayerName(1, "Renamed");
    EXPECT_EQ(svc.document().layerAt(1)->name(), std::string("Renamed"));

    // Undo #1: should undo rename, NOT remove the layer
    svc.undo();
    ASSERT_EQ(svc.document().layerCount(), 2u);
    EXPECT_EQ(svc.document().layerAt(1)->name(), std::string("Layer 1"));

    // Redo: should redo rename
    svc.redo();
    ASSERT_EQ(svc.document().layerCount(), 2u);
    EXPECT_EQ(svc.document().layerAt(1)->name(), std::string("Renamed"));

    // Undo twice: 1) undo rename, 2) undo addLayer -> back to background only
    svc.undo();
    svc.undo();
    EXPECT_EQ(svc.document().layerCount(), 1u);
}


TEST(AppService_SetLayerName, ThrowsOnLockedLayerAndDoesNotRename)
{
    app::AppService svc(nullptr);
    svc.newDocument({32, 32}, 72.f);

    addOneEditableLayer(svc, "Layer 1");
    ASSERT_EQ(svc.document().layerCount(), 2u);

    svc.setLayerLocked(1, true);
    ASSERT_TRUE(svc.document().layerAt(1)->locked());

    EXPECT_THROW(svc.setLayerName(1, "ShouldFail"), std::runtime_error);
    EXPECT_EQ(svc.document().layerAt(1)->name(), std::string("Layer 1"));
}

TEST(AppService_SetLayerName, NoOpIfSameNameDoesNotPushHistory)
{
    app::AppService svc(nullptr);
    svc.newDocument({32, 32}, 72.f);

    addOneEditableLayer(svc, "Layer 1");
    ASSERT_EQ(svc.document().layerCount(), 2u);

    // No-op rename (same name)
    svc.setLayerName(1, "Layer 1");
    EXPECT_EQ(svc.document().layerAt(1)->name(), std::string("Layer 1"));

    // If no command was pushed, the next undo should undo addLayer and remove the layer
    svc.undo();
    EXPECT_EQ(svc.document().layerCount(), 1u);
}

TEST(AppService_Layers, MergeLayerDown_BackgroundThrows)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    EXPECT_THROW(app->mergeLayerDown(0), std::runtime_error);
}

TEST(AppService_Layers, ReorderLayer_ActiveLayerFollowsMovedLayer)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;

    app->addLayer(spec); // idx 1
    app->addLayer(spec); // idx 2
    app->addLayer(spec); // idx 3

    app->setActiveLayer(3);
    ASSERT_EQ(app->activeLayer(), 3u);

    // Move active layer from 3 to 1
    app->reorderLayer(3, 1);

    EXPECT_EQ(app->activeLayer(), 1u);
}

TEST(AppService_Layers, AddLayer_UsesSpecSize)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app::LayerSpec spec{};
    spec.name = "small";
    spec.color = 0u;
    spec.width = 10;
    spec.height = 20;

    app->addLayer(spec);

    ASSERT_EQ(app->document().layerCount(), 2u);
    auto l = app->document().layerAt(1);
    ASSERT_NE(l, nullptr);
    ASSERT_NE(l->image(), nullptr);
    EXPECT_EQ(l->image()->width(), 10);
    EXPECT_EQ(l->image()->height(), 20);
}

TEST(AppService_Layers, AddImageLayer_UsesImageSize)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);


    ImageBuffer img(12, 7);
    img.fill(0xFF00FFFFu);

    app->addImageLayer(img, "img");

    ASSERT_EQ(app->document().layerCount(), 2u);
    auto l = app->document().layerAt(1);
    ASSERT_NE(l, nullptr);
    ASSERT_NE(l->image(), nullptr);
    EXPECT_EQ(l->image()->width(), 12);
    EXPECT_EQ(l->image()->height(), 7);
}

