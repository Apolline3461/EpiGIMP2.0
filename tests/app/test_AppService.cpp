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

TEST(AppService, documentReturnsConstRef)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    const Document* p1 = &app->document();
    const Document* p2 = &app->document();
    EXPECT_EQ(p1, p2);
}

TEST(AppService, newDocumentInitialState_Default) {
    const auto app = makeApp();

    app->newDocument(app::Size{640, 480}, 72.F);

    const Document& doc = app->document();
    EXPECT_EQ(doc.width(), 640);
    EXPECT_EQ(doc.height(), 480);
    EXPECT_FLOAT_EQ(doc.dpi(), 72.F);

    EXPECT_EQ(app->activeLayer(), 0);

    EXPECT_FALSE(app->canUndo());
    EXPECT_FALSE(app->canRedo());
}

TEST(AppService, LayerIds_AreUnique) {
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

TEST(AppService, LayerIds_ResetOnNewDocument) {
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

TEST(AppService, NewDocumentInitialLayer_CountIsOne) {

    const auto app = makeApp();

    app->newDocument(app::Size{100, 200}, 72.F);

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

    EXPECT_EQ(bgLayer->image()->getPixel(0,0), 0xFFFFFFFFu);
    EXPECT_EQ(bgLayer->image()->getPixel(99,199), 0xFFFFFFFFu);
}

TEST(AppService, RemoveLayer_WhenLocked_Throws) {
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    EXPECT_THROW(app->removeLayer(0), std::runtime_error);
    EXPECT_EQ(app->document().layerCount(), 1);
}

TEST(AppService, RemoveLayer_AfterUnlock_AllowsEmptyDocument) {
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    app->setLayerLocked(0, false);
    app->removeLayer(0);

    EXPECT_EQ(app->document().layerCount(), 0);
}

TEST(AppService, documentChanged_EmittedOnNewDocument) {
    const auto app = makeApp();

    int hits = 0;
    app->documentChanged.connect([&]() { hits++; });

    app->newDocument(app::Size{10, 10}, 72.f);

    EXPECT_EQ(hits, 1);
}

TEST(AppService, ActiveLayerSet_ValidIndex)
{
    const auto app = makeApp();
    app::LayerSpec spec{};

    app->newDocument(app::Size{10, 10}, 72.f);
    app->addLayer(spec);
    app->addLayer(spec);

    app->setActiveLayer(2);
    EXPECT_EQ(app->activeLayer(), 2);
}

TEST(AppService, ActiveLayerSet_OutOfRangeThrows)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f);

    EXPECT_THROW(app->setActiveLayer(1), std::out_of_range);
}

TEST(AppService, Open_CallsStorage) {
    SpyStorage* spy = nullptr;
    auto app = makeApp(&spy);

    app->open("foo.epg");

    ASSERT_NE(spy, nullptr);
    EXPECT_TRUE(spy->openCalled);
    EXPECT_EQ(spy->lastOpenPath, "foo.epg");
}
TEST(AppService, Save_CallsStorage) {
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

TEST(AppService, exportImage_CallsStorage) {
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