#pragma once

#include <memory>
#include <string>

#include "app/AppService.hpp"
#include "core/Document.hpp"
#include "io/IStorage.hpp"

class SpyStorage final : public IStorage
{
public:
    bool openCalled = false;
    bool saveCalled = false;
    bool exportCalled = false;

    std::string lastOpenPath;
    std::string lastSavePath;
    std::string lastExportPath;

    const Document* lastSavedDoc = nullptr;
    const Document* lastExportedDoc = nullptr;

    // Optionnel : pour contrôler ce que open() renvoie
    std::unique_ptr<Document> nextOpenDocument = std::make_unique<Document>(1, 1, 72.f);

    io::epg::OpenResult open(const std::string& path) override
    {
        openCalled = true;
        lastOpenPath = path;

        io::epg::OpenResult result;
        result.success = true;
        result.document = std::move(nextOpenDocument);
        if (!result.document)
            result.document = std::make_unique<Document>(1, 1, 72.f);
        return result;
    }

    void save(const Document& doc, const std::string& path) override
    {
        saveCalled = true;
        lastSavePath = path;
        lastSavedDoc = &doc;
    }

    void exportImage(const Document& doc, const std::string& path) override
    {
        exportCalled = true;
        lastExportPath = path;
        lastExportedDoc = &doc;
    }
};

inline std::unique_ptr<app::AppService> makeAppWithSpy(SpyStorage*& outSpy)
{
    auto spy = std::make_unique<SpyStorage>();
    outSpy = spy.get();
    return std::make_unique<app::AppService>(std::move(spy));
}

inline std::unique_ptr<app::AppService> makeApp()
{
    SpyStorage* ignored = nullptr;
    return makeAppWithSpy(ignored);
}