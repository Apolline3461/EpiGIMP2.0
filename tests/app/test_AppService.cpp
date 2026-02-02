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
    EXPECT_TRUE(bgLayer->locked());
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

    Selection::Rect r{2, 3, 4, 2};
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

    ASSERT_TRUE(app->document().layerAt(0)->locked());

    app->setLayerLocked(0, false);
    EXPECT_FALSE(app->document().layerAt(0)->locked());
    EXPECT_TRUE(app->canUndo());
    EXPECT_FALSE(app->canRedo());

    app->undo();
    EXPECT_TRUE(app->document().layerAt(0)->locked());
    EXPECT_FALSE(app->canUndo());
    EXPECT_TRUE(app->canRedo());

    app->redo();
    EXPECT_FALSE(app->document().layerAt(0)->locked());
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

    app->setLayerLocked(0, false); // +1
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

    // background est déjà locked=true
    app->setLayerLocked(0, true);

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
