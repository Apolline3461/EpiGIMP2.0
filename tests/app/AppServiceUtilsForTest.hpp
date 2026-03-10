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