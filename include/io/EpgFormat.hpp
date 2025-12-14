#pragma once

#include <openssl/sha.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "core/document.hpp"
#include "core/ImageBuffer.h"
#include "io/EpgTypes.hpp"
using json = nlohmann::json;

void pngWriteCallback(void* context, void* data, int size);
std::string getCurrentTimestampUTC();
std::string formatLayerId(size_t index);
std::unique_ptr<ImageBuffer> decodePngToImageBuffer(const std::vector<unsigned char>& pngData);

class ZipEpgStorage
{
   public:
    ZipEpgStorage() = default;
    ~ZipEpgStorage() = default;

    // Expose legacy nested type names for compatibility with EpgJson.hpp
    using Transform = ::Transform;
    using Bounds = ::Bounds;
    using TextData = ::TextData;
    using ManifestLayer = ::ManifestLayer;
    using LayerGroup = ::LayerGroup;
    using IOConfig = ::IOConfig;
    using Metadata = ::Metadata;
    using ManifestInfo = ::ManifestInfo;
    using Canvas = ::Canvas;
    using Manifest = ::Manifest;
    using Color = ::Color;

    // Interface IStorage-like methods
    OpenResult open(const std::string& path);
    void save(const Document& doc, const std::string& path);
    void exportPng(const Document& doc, const std::string& path);

    // Helpers (made public for unit testing)
    Manifest loadManifestFromZip(zip_t* zipHandle) const;
    void writeLayersToZip(zip_t* zipHandle, Manifest& m, const Document& doc) const;
    void writeManifestToZip(zip_t* zipHandle, const Manifest& m) const;
    Manifest createManifestFromDocument(const Document& doc) const;
    std::vector<unsigned char> composePreviewRGBA(const Document& doc, int& outW, int& outH) const;
    std::vector<unsigned char> encodePngToVector(const unsigned char* rgba, int w, int h,
                                                 int channels, int stride) const;
    void writePreviewToZip(zip_t* zipHandle, const std::vector<unsigned char>& pngData) const;
    void generatePreview(const Document& doc, zip_t* zipHandle) const;
    std::vector<unsigned char> composeFlattenedRGBA(const Document& doc) const;

   private:
    Manifest parseManifest(const std::string& jsonText, std::vector<std::string>& warnings) const;
    void validateManifest(const Manifest& m) const;

    // ZIP helpers
    std::vector<unsigned char> readFileFromZip(zip_t* zipHandle, const std::string& filename) const;
    void writeFileToZip(zip_t* zipHandle, const std::string& filename, const void* data,
                        size_t size) const;

    // Checksums
    std::string computeSHA256(const void* data, size_t size) const;
    bool verifySHA256(const void* data, size_t size, const std::string& expectedHash) const;

    // Document/Manifest conversion
    std::unique_ptr<Document> createDocumentFromManifest(const Manifest& manifest,
                                                         zip_t* zipHandle) const;
};

// Abstract storage interface
class IStorage
{
   public:
    virtual ~IStorage() = default;
    virtual OpenResult open(const std::string& path) = 0;
    virtual void save(const Document& doc, const std::string& path) = 0;
    virtual void exportPng(const Document& doc, const std::string& path) = 0;
};

#include "io/EpgJson.hpp"
