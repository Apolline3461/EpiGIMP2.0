//
// Created by apolline on 21/01/2026.
//

#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "app/AppService.hpp"
#include "io/IStorage.hpp"
#include "io/EpgTypes.hpp"
#include "core/Document.hpp"

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

    io::epg::OpenResult openResultToReturn{};

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

    void exportPng(const Document& doc, const std::string& path) override {
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

    app->newDocument(app::Size{10, 10}, 72.f);
    app->addLayer();
    app->addLayer();

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

TEST(AppService, ExportPng_CallsStorage) {
    SpyStorage* spy = nullptr;
    auto app = makeApp(&spy);

    app->newDocument(app::Size{10, 10}, 72.f);
    const Document* current = &app->document();

    app->exportPng("out.png");

    ASSERT_NE(spy, nullptr);
    EXPECT_TRUE(spy->exportCalled);
    EXPECT_EQ(spy->lastExportPath, "out.png");
    EXPECT_EQ(spy->lastExportedDoc, current);

}