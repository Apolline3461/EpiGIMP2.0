#include <stb_image.h>
#include <stb_image_write.h>
#include <zip.h>
#include <zipconf.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <ios>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/Document.hpp"
#include "core/Layer.hpp"
#include "io/EpgFormat.hpp"
#include "io/EpgJson.hpp"
#include "io/EpgTypes.hpp"
#include "io/Logger.hpp"

#include <nlohmann/json.hpp>
#include <openssl/sha.h>

using json = nlohmann::json;
using namespace io::epg;

// ----------------- ZIP helpers --------------------------------------------

struct FreeDeleter
{
    void operator()(void* p) const noexcept
    {
        std::free(p);
    }
};

struct ZipSourceDeleter
{
    void operator()(zip_source_t* zip) const noexcept
    {
        if (zip)
            zip_source_free(zip);
    }
};

std::vector<unsigned char> ZipEpgStorage::readFileFromZip(zip_t* zip,
                                                          const std::string& filename) const
{
    if (!zip)
        throw std::runtime_error("Handle ZIP invalide");

    struct zip_stat st;
    zip_stat_init(&st);

    if (zip_stat(zip, filename.c_str(), 0, &st) != 0)
        throw std::runtime_error("Fichier introuvable dans le ZIP : " + filename);

    zip_file_t* zf = zip_fopen(zip, filename.c_str(), 0);
    if (!zf)
        throw std::runtime_error("Impossible d'ouvrir " + filename + " dans le ZIP");

    if (st.size > 0 && st.size > static_cast<zip_uint64_t>(std::numeric_limits<size_t>::max()))
    {
        zip_fclose(zf);
        throw std::runtime_error("Fichier trop grand dans le ZIP: " + filename);
    }

    std::vector<unsigned char> data;
    data.resize(static_cast<size_t>(st.size));
    zip_int64_t const read = zip_fread(zf, data.data(), st.size);
    if (read < 0 || static_cast<zip_uint64_t>(read) != st.size)
    {
        zip_fclose(zf);
        throw std::runtime_error("Erreur de lecture depuis le ZIP pour: " + filename +
                                 " (lus=" + std::to_string(read) +
                                 ", attendu=" + std::to_string(st.size) + ")");
    }
    zip_fclose(zf);

    return data;
}

void ZipEpgStorage::writeFileToZip(zip_t* zip, const std::string& filename, const void* data,
                                   size_t size) const
{
    if (!zip)
        throw std::runtime_error("Handle ZIP invalide");

    if (!data || size == 0)
        throw std::runtime_error("Données invalides pour " + filename);

    zip_int64_t const existing = zip_name_locate(zip, filename.c_str(), 0);
    if (existing >= 0)
    {
        if (zip_delete(zip, existing) < 0)
        {
            epg::log_warn("Impossible de supprimer l'ancien fichier: " + filename);
        }
    }

    std::unique_ptr<void, FreeDeleter> bufferCopy(std::malloc(size));
    if (!bufferCopy)
        throw std::runtime_error("Échec d'allocation mémoire pour " + filename);

    std::memcpy(bufferCopy.get(), data, size);

    std::unique_ptr<zip_source_t, ZipSourceDeleter> source(
        zip_source_buffer(zip, bufferCopy.get(), size, 1));
    if (!source)
        throw std::runtime_error("zip_source_buffer failed: " + std::string(zip_strerror(zip)));
    (void)bufferCopy.release();

    zip_int64_t const index = zip_file_add(zip, filename.c_str(), source.get(), ZIP_FL_ENC_UTF_8);
    if (index < 0)
    {
        throw std::runtime_error("Impossible d'ajouter le fichier dans le ZIP: " + filename +
                                 " - " + std::string(zip_strerror(zip)));
    }
    (void)source.release();
}

// ----------------- SHA256 -------------------------------------------------

std::string ZipEpgStorage::computeSHA256(const void* data, size_t size) const
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data), size, hash);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::setw(2) << static_cast<int>(hash[i]);
    return ss.str();
}

bool ZipEpgStorage::verifySHA256(const void* data, size_t size, const std::string& expected) const
{
    return computeSHA256(data, size) == expected;
}

ZipEpgStorage::Manifest ZipEpgStorage::parseManifest(const std::string& jsonText,
                                                     std::vector<std::string>& warnings) const
{
    json const j = json::parse(jsonText);
    Manifest m = j.get<Manifest>();

    for (const auto& L : m.layers)
    {
        if (L.id.empty())
            warnings.push_back("Layer sans ID détecté");
    }

    return m;
}

void ZipEpgStorage::validateManifest(const Manifest& m) const
{
    if (m.canvas.width <= 0 || m.canvas.height <= 0)
        throw std::runtime_error("Canvas invalide (dimensions <= 0)");
    if (m.canvas.width > 65535 || m.canvas.height > 65535)
        throw std::runtime_error("Canvas trop grand (max 65535x65535)");

    for (const auto& L : m.layers)
    {
        if (L.opacity < 0.f || L.opacity > 1.f)
            throw std::runtime_error("Opacité invalide pour le layer : " + L.id);
        if (L.id.empty())
            throw std::runtime_error("Layer sans ID");
    }
}

ZipEpgStorage::Manifest ZipEpgStorage::createManifestFromDocument(const Document& doc) const
{
    Manifest m;
    m.epgVersion = 1;

    // Canvas
    m.canvas.name = "EpiGimp2.0";
    m.canvas.width = doc.width();
    m.canvas.height = doc.height();
    m.canvas.dpi = doc.dpi();
    m.canvas.colorSpace = "sRGB";
    m.canvas.background = Color{255, 255, 255, 0};

    // IO defaults
    m.io.pixelFormatStorage = "RGBA8_unorm_straight";
    m.io.pixelFormatRuntime = "ARGB32_premultiplied";
    m.io.colorDepth = 8;
    m.io.compression = "png";

    // Metadata
    std::string const now = getCurrentTimestampUTC();
    m.metadata.author = "EpiGimp User";
    m.metadata.description = "Document créé avec EpiGimp";
    m.metadata.createdUtc = now;
    m.metadata.modifiedUtc = now;

    // Layers
    for (int i = 0; i < doc.layerCount(); ++i)
    {
        std::string const layerId = formatLayerId(i);
        ManifestLayer L;
        L.id = layerId;
        L.name = doc.layerAt(i)->name();
        L.type = LayerType::Raster;
        L.visible = doc.layerAt(i)->visible();
        L.locked = doc.layerAt(i)->locked();
        L.opacity = doc.layerAt(i)->opacity();
        L.blendMode = BlendMode::Normal;
        L.path = "layers/" + layerId + ".png";
        L.sha256 = "";
        L.transform = Transform{};
        L.bounds.x = 0;
        L.bounds.y = 0;
        L.bounds.width = doc.width();
        L.bounds.height = doc.height();
        m.layers.push_back(L);
    }

    return m;
}

std::unique_ptr<Document> ZipEpgStorage::createDocumentFromManifest(const Manifest& m,
                                                                    zip_t* handle) const
{
    auto doc = std::make_unique<Document>(m.canvas.width, m.canvas.height, m.canvas.dpi);

    for (const auto& lm : m.layers)
    {
        try
        {
            std::vector<unsigned char> pngData = readFileFromZip(handle, lm.path);
            if (!lm.sha256.empty())
            {
                if (!verifySHA256(pngData.data(), pngData.size(), lm.sha256))
                    epg::log_warn(std::string("checksum SHA256 mismatch for ") + lm.path);
            }
            auto buf = decodePngToImageBuffer(pngData);
            std::shared_ptr<ImageBuffer> sharedBuf;
            if (buf)
                sharedBuf = std::shared_ptr<ImageBuffer>(std::move(buf));

            auto layer = std::make_shared<Layer>(std::stoull(lm.id), lm.name, sharedBuf, lm.visible,
                                                 lm.locked, lm.opacity);

            doc->addLayer(layer);
        }
        catch (const std::exception& e)
        {
            epg::log_warn(std::string("Avertissement: impossible de charger le layer ") + lm.name +
                          ": " + e.what());
        }
    }

    return doc;
}

// Load and validate manifest from the opened ZIP
ZipEpgStorage::Manifest ZipEpgStorage::loadManifestFromZip(zip_t* zipHandle) const
{
    if (!zipHandle)
        throw std::runtime_error("Handle ZIP invalide");

    std::vector<std::string> warnings;
    std::vector<unsigned char> const projectJsonVec = readFileFromZip(zipHandle, "project.json");
    std::string projectJson(projectJsonVec.begin(), projectJsonVec.end());

    Manifest const m = parseManifest(projectJson, warnings);
    for (const auto& w : warnings)
        epg::log_warn(std::string("Manifest warning: ") + w);

    validateManifest(m);
    return m;
}

// Write each layer PNG into ZIP and update the manifest's layers' sha256 and manifest_info.entries
void ZipEpgStorage::writeLayersToZip(zip_t* zipHandle, Manifest& m, const Document& doc) const
{
    if (!zipHandle)
        throw std::runtime_error("Handle ZIP invalide");

    // Keep generated PNG buffers alive until written into zip
    std::vector<std::vector<unsigned char>> pngBuffers;
    pngBuffers.reserve(m.layers.size());

    std::vector<std::pair<std::string, std::string>> manifestEntries;

    for (size_t i = 0; i < m.layers.size(); ++i)
    {
        auto& L = m.layers[i];

        if (static_cast<int>(i) >= doc.layerCount())
            throw std::runtime_error(
                "Incohérence: nombre de calques différent entre Document et Manifest");

        const auto layerPtr = doc.layerAt(static_cast<int>(i));
        if (!layerPtr->image())
            throw std::runtime_error("Layer " + layerPtr->name() + " n'a pas de pixels");

        const ImageBuffer& img = *layerPtr->image();

        int stride = img.strideBytes();
        if (stride == 0)
            stride = img.width() * 4;

        // Create a buffer and let stbi write into it
        pngBuffers.emplace_back();
        std::vector<unsigned char>& buffer = pngBuffers.back();

        stbi_write_png_to_func(pngWriteCallback, &buffer, static_cast<int>(img.width()),
                               static_cast<int>(img.height()), 4, img.data(), stride);

        if (buffer.empty())
            throw std::runtime_error("Impossible d'encoder le PNG pour le layer " + L.path);

        if (buffer.size() < 8 || buffer[0] != 137 || buffer[1] != 80 || buffer[2] != 78 ||
            buffer[3] != 71)
        {
            throw std::runtime_error("Signature PNG invalide pour " + layerPtr->name());
        }
        std::string const sha = computeSHA256(buffer.data(), buffer.size());
        writeFileToZip(zipHandle, L.path, buffer.data(), buffer.size());
        m.layers[i].sha256 = sha;
        manifestEntries.emplace_back(L.path, sha);
    }

    m.manifestInfo.entries = manifestEntries;
}

void ZipEpgStorage::writeManifestToZip(zip_t* zipHandle, const Manifest& m) const
{
    if (!zipHandle)
        throw std::runtime_error("Handle ZIP invalide");

    const json j = m;
    std::string pj = j.dump(4);
    writeFileToZip(zipHandle, "project.json", pj.data(), pj.size());
}

void ZipEpgStorage::generatePreview(const Document& doc, zip_t* handle) const
{
    if (doc.layerCount() == 0)
        return;

    int w = 0, h = 0;
    // Compose RGBA preview buffer
    std::vector<unsigned char> preview = composePreviewRGBA(doc, w, h);
    if (preview.empty() || w <= 0 || h <= 0)
        return;

    // Encode to PNG
    std::vector<unsigned char> pngData = encodePngToVector(preview.data(), w, h, 4, w * 4);
    if (pngData.empty())
    {
        epg::log_warn("generatePreview: échec de l'encodage PNG");
        return;
    }

    // Validate signature and write
    try
    {
        writePreviewToZip(handle, pngData);
    }
    catch (const std::exception& e)
    {
        epg::log_warn("Échec d'écriture du preview dans le ZIP: " + std::string(e.what()));
    }
}

std::vector<unsigned char> ZipEpgStorage::composePreviewRGBA(const Document& doc, int& outW,
                                                             int& outH) const
{
    constexpr int previewMax = 256;

    const int docW = doc.width();
    const int docH = doc.height();

    if (docW <= 0 || docH <= 0)
    {
        outW = 0;
        outH = 0;
        return {};
    }

    int const pW = std::min(previewMax, docW);
    int const pH = std::min(previewMax, docH);

    float const scaleX = static_cast<float>(pW) / static_cast<float>(docW);
    float const scaleY = static_cast<float>(pH) / static_cast<float>(docH);
    float const scale = std::min(scaleX, scaleY);

    int const w = std::max(1, static_cast<int>(static_cast<float>(docW) * scale));
    int const h = std::max(1, static_cast<int>(static_cast<float>(docH) * scale));

    std::vector<unsigned char> preview(static_cast<size_t>(w) * static_cast<size_t>(h) * 4u, 0);

    for (int i = 0; i < doc.layerCount(); ++i)
    {
        auto layerPtr = doc.layerAt(i);
        if (!layerPtr || !layerPtr->visible() || !layerPtr->image())
            continue;

        const ImageBuffer& img = *layerPtr->image();
        const float layerOpacity = layerPtr->opacity();

        for (int py = 0; py < h; ++py)
        {
            int const srcY = std::min(
                img.height() - 1, std::max(0, static_cast<int>(static_cast<float>(py) / scale)));
            for (int px = 0; px < w; ++px)
            {
                int const srcX = std::min(
                    img.width() - 1, std::max(0, static_cast<int>(static_cast<float>(px) / scale)));

                const int dstIdx = (py * w + px) * 4;
                const int srcIdx = srcY * img.strideBytes() + srcX * 4;

                const unsigned char sr = img.data()[srcIdx + 0];
                const unsigned char sg = img.data()[srcIdx + 1];
                const unsigned char sb = img.data()[srcIdx + 2];
                const unsigned char sa = img.data()[srcIdx + 3];

                const float srcAlphaNorm = (static_cast<float>(sa) / 255.0f) * layerOpacity;

                if (srcAlphaNorm <= 0.0001f)
                    continue;

                const float dstAlphaNorm = static_cast<float>(preview[dstIdx + 3]) / 255.0f;
                const float outAlpha = srcAlphaNorm + dstAlphaNorm * (1.0f - srcAlphaNorm);

                if (outAlpha > 0.0001f)
                {
                    for (int c = 0; c < 3; ++c)
                    {
                        const float srcVal =
                            (c == 0 ? static_cast<float>(sr)
                                    : (c == 1 ? static_cast<float>(sg) : static_cast<float>(sb)));
                        const auto dstVal = static_cast<float>(preview[dstIdx + c]);

                        const float numerator =
                            srcVal * srcAlphaNorm + dstVal * dstAlphaNorm * (1.0f - srcAlphaNorm);
                        const float outVal = numerator / outAlpha;

                        preview[dstIdx + c] = static_cast<unsigned char>(
                            std::round(std::min(255.0f, std::max(0.0f, outVal))));
                    }
                    preview[dstIdx + 3] =
                        static_cast<unsigned char>(std::round(std::min(1.0f, outAlpha) * 255.0f));
                }
            }
        }
    }

    outW = w;
    outH = h;
    return preview;
}

std::vector<unsigned char> ZipEpgStorage::encodePngToVector(const unsigned char* rgba, int w, int h,
                                                            int channels, int stride) const
{
    std::vector<unsigned char> out;
    stbi_write_png_to_func(pngWriteCallback, &out, w, h, channels, rgba, stride);
    return out;
}

void ZipEpgStorage::writePreviewToZip(zip_t* zipHandle,
                                      const std::vector<unsigned char>& pngData) const
{
    if (pngData.size() < 8 || memcmp(pngData.data(), kPngSignature, 8) != 0)
        throw std::runtime_error("generatePreview: PNG trop petit");

    if (std::memcmp(pngData.data(), kPngSignature, 8) != 0)
    {
        std::ostringstream oss;
        oss << "generatePreview: signature PNG invalide: ";
        oss << std::hex << std::setfill('0');
        for (int i = 0; i < 8; ++i)
            oss << std::setw(2) << static_cast<int>(pngData[i]) << " ";
        epg::log_warn(oss.str());
        return;
    }

    writeFileToZip(zipHandle, "preview.png", pngData.data(), pngData.size());
}

ZipEpgStorage::OpenResult ZipEpgStorage::open(const std::string& path)
{
    int err = 0;
    zip_t* raw = zip_open(path.c_str(), ZIP_RDONLY, &err);
    if (!raw)
    {
        return {false, "Impossible d'ouvrir le ZIP (code: " + std::to_string(err) + ")", nullptr};
    }
    ZipHandle const zip(raw);

    ZipEpgStorage::OpenResult res;
    try
    {
        // Load and validate manifest from the ZIP
        Manifest const manifest = loadManifestFromZip(zip.get());

        res.document = createDocumentFromManifest(manifest, zip.get());
        res.success = true;
    }
    catch (const std::exception& e)
    {
        res.success = false;
        res.errorMessage = e.what();
    }

    // ZipHandle will auto-close
    return res;
}

void ZipEpgStorage::save(const Document& doc, const std::string& path)
{
    int err = 0;
    zip_t* raw = zip_open(path.c_str(), ZIP_TRUNCATE | ZIP_CREATE, &err);
    if (!raw)
        throw std::runtime_error("Impossible de créer le ZIP (code: " + std::to_string(err) + ")");
    ZipHandle zip(raw);

    try
    {
        Manifest m = createManifestFromDocument(doc);

        // Write layers and update manifest with computed SHA256s
        writeLayersToZip(zip.get(), m, doc);

        m.metadata.modifiedUtc = getCurrentTimestampUTC();
        m.manifestInfo.fileCount = static_cast<int>(1 + m.manifestInfo.entries.size());
        m.manifestInfo.generatedUtc = getCurrentTimestampUTC();

        // Write manifest file
        writeManifestToZip(zip.get(), m);
        generatePreview(doc, zip.get());
    }
    catch (...)
    {
        throw;
    }
}

// ----------------- EXPORT PNG --------------------------------------------

void ZipEpgStorage::exportPng(const Document& doc, const std::string& path)
{
    if (doc.layerCount() == 0)
        throw std::runtime_error("Document vide, impossible d'exporter");

    std::vector<unsigned char> out = composeFlattenedRGBA(doc);

    if (out.empty())
        throw std::runtime_error("Impossible de composer l'image pour l'export PNG");

    if (!stbi_write_png(path.c_str(), doc.width(), doc.height(), 4, out.data(), doc.width() * 4))
        throw std::runtime_error("Impossible d'écrire le fichier PNG: " + path);
}

std::vector<unsigned char> ZipEpgStorage::composeFlattenedRGBA(const Document& doc) const
{
    const int docW = doc.width();
    const int docH = doc.height();

    if (docW <= 0 || docH <= 0)
        return {};

    std::vector<unsigned char> out(static_cast<size_t>(docW) * static_cast<size_t>(docH) * 4u, 0);
    for (int i = 0; i < doc.layerCount(); ++i)
    {
        auto layerPtr = doc.layerAt(i);
        if (!layerPtr || !layerPtr->visible() || !layerPtr->image())
            continue;
        const ImageBuffer& img = *layerPtr->image();

        for (int y = 0; y < docH && y < img.height(); ++y)
        {
            for (int x = 0; x < docW && x < img.width(); ++x)
            {
                int const dstIdx = (y * docW + x) * 4;
                int const srcIdx = (y * img.width() + x) * 4;

                float const alpha =
                    (static_cast<float>(img.data()[srcIdx + 3]) / 255.0f) * layerPtr->opacity();

                for (int c = 0; c < 3; ++c)
                {
                    auto const src = static_cast<float>(img.data()[srcIdx + c]);
                    auto const dst = static_cast<float>(out[dstIdx + c]);
                    out[dstIdx + c] =
                        static_cast<unsigned char>(src * alpha + dst * (1.0f - alpha));
                }

                out[dstIdx + 3] = std::max<unsigned char>(
                    out[dstIdx + 3], static_cast<unsigned char>(alpha * 255.0f));
            }
        }
    }

    return out;
}
