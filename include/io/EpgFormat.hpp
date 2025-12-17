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
#include "io/EpgZip.hpp"
#include "io/EpgTypes.hpp"
using json = nlohmann::json;

void pngWriteCallback(void* context, void* data, int size);
std::string getCurrentTimestampUTC();
std::string formatLayerId(size_t index);
std::unique_ptr<ImageBuffer> decodePngToImageBuffer(const std::vector<unsigned char>& pngData);

// Storage interface (extracted to separate header)
#include "io/IStorage.hpp"

class ZipEpgStorage : public IStorage
{
   public:
    ZipEpgStorage() = default;
    ~ZipEpgStorage() = default;

    // Expose legacy nested type names for compatibility with EpgJson.hpp
    using Transform = io::epg::Transform;
    using Bounds = io::epg::Bounds;
    using TextData = io::epg::TextData;
    using ManifestLayer = io::epg::ManifestLayer;
    using LayerGroup = io::epg::LayerGroup;
    using IOConfig = io::epg::IOConfig;
    using Metadata = io::epg::Metadata;
    using ManifestInfo = io::epg::ManifestInfo;
    using Canvas = io::epg::Canvas;
    using Manifest = io::epg::Manifest;
    using Color = io::epg::Color;
    using OpenResult = io::epg::OpenResult;

    // Interface IStorage-like methods
    OpenResult open(const std::string& path) override;
    void save(const Document& doc, const std::string& path) override;
    void exportPng(const Document& doc, const std::string& path) override;

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
