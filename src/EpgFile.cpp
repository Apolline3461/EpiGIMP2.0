#include "EpgFile.hpp"

#include <openssl/sha.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <zip.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "document.hpp"
#include "imagebuffer.hpp"
#include "layer.hpp"

using json = nlohmann::json;

// ----------------- utilitaires --------------------------------------------

static void pngWriteCallback(void* context, void* data, int size)
{
    auto* buffer = reinterpret_cast<std::vector<unsigned char>*>(context);
    unsigned char* bytes = reinterpret_cast<unsigned char*>(data);
    buffer->insert(buffer->end(), bytes, bytes + size);
}

static std::string getCurrentTimestampUTC()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::gmtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

static std::string formatLayerId(size_t index)
{
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << (index + 1);
    return oss.str();
}

// ----------------- ZIP helpers --------------------------------------------

std::string ZipEpgStorage::readFileFromZip(void* handle, const std::string& filename) const
{
    zip_t* zip = reinterpret_cast<zip_t*>(handle);
    if (!zip)
        throw std::runtime_error("Handle ZIP invalide");

    struct zip_stat st;
    zip_stat_init(&st);

    if (zip_stat(zip, filename.c_str(), 0, &st) != 0)
        throw std::runtime_error("Fichier introuvable dans le ZIP : " + filename);

    zip_file_t* zf = zip_fopen(zip, filename.c_str(), 0);
    if (!zf)
        throw std::runtime_error("Impossible d'ouvrir " + filename + " dans le ZIP");

    std::string data;
    data.resize(st.size);
    zip_fread(zf, data.data(), st.size);
    zip_fclose(zf);

    return data;
}

void ZipEpgStorage::writeFileToZip(void* handle, const std::string& filename, const void* data,
                                   size_t size) const
{
    zip_t* zip = reinterpret_cast<zip_t*>(handle);
    if (!zip)
        throw std::runtime_error("Handle ZIP invalide");

    zip_source_t* source = zip_source_buffer(zip, data, size, 0);
    if (!source)
        throw std::runtime_error("zip_source_buffer failed : " + std::string(zip_strerror(zip)));

    if (zip_file_add(zip, filename.c_str(), source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0)
    {
        zip_source_free(source);
        throw std::runtime_error("Impossible d'ajouter le fichier dans le ZIP : " + filename);
    }
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

// ----------------- PNG decoding / encoding helpers -------------------------

// Décode PNG depuis la mémoire (raw PNG bytes) vers ImageBuffer
static std::unique_ptr<ImageBuffer> decodePngToImageBuffer(const std::string& pngData)
{
    int width = 0, height = 0, channels = 0;
    unsigned char* decoded =
        stbi_load_from_memory(reinterpret_cast<const unsigned char*>(pngData.data()),
                              static_cast<int>(pngData.size()), &width, &height, &channels, 4);

    if (!decoded)
        throw std::runtime_error("stb_image: impossible de décoder le PNG en mémoire");

    auto buf = std::make_unique<ImageBuffer>();
    buf->width = width;
    buf->height = height;
    buf->stride = width * 4;
    buf->rgba.assign(decoded, decoded + (width * height * 4));

    stbi_image_free(decoded);
    return buf;
}


// ----------------- JSON parsing / sérialisation ---------------------------

ZipEpgStorage::Manifest ZipEpgStorage::parseManifest(const std::string& jsonText,
                                                     std::vector<std::string>& warnings) const
{
    json j = json::parse(jsonText);

    Manifest m;
    m.epg_version = j.value("epg_version", 1);

    // Canvas
    if (j.contains("canvas"))
    {
        auto& jc = j["canvas"];
        m.canvas.name = jc.value("name", m.canvas.name);
        m.canvas.width = jc.value("width", m.canvas.width);
        m.canvas.height = jc.value("height", m.canvas.height);
        m.canvas.dpi = jc.value("dpi", m.canvas.dpi);
        m.canvas.color_space = jc.value("color_space", m.canvas.color_space);
        if (jc.contains("background"))
        {
            auto& bg = jc["background"];
            m.canvas.background.r = bg.value("r", m.canvas.background.r);
            m.canvas.background.g = bg.value("g", m.canvas.background.g);
            m.canvas.background.b = bg.value("b", m.canvas.background.b);
            m.canvas.background.a = bg.value("a", m.canvas.background.a);
        }
    }

    // IO
    if (j.contains("io"))
    {
        auto& ji = j["io"];
        m.io.pixel_format_storage = ji.value("pixel_format_storage", m.io.pixel_format_storage);
        m.io.pixel_format_runtime = ji.value("pixel_format_runtime", m.io.pixel_format_runtime);
        m.io.color_depth = ji.value("color_depth", m.io.color_depth);
        m.io.compression = ji.value("compression", m.io.compression);
    }

    // Metadata
    if (j.contains("metadata"))
    {
        auto& jm = j["metadata"];
        m.metadata.author = jm.value("author", m.metadata.author);
        m.metadata.description = jm.value("description", m.metadata.description);
        m.metadata.created_utc = jm.value("created_utc", m.metadata.created_utc);
        m.metadata.modified_utc = jm.value("modified_utc", m.metadata.modified_utc);
        if (jm.contains("tags"))
            m.metadata.tags = jm["tags"].get<std::vector<std::string>>();
        m.metadata.license = jm.value("license", m.metadata.license);
    }

    // Manifest integrity
    if (j.contains("manifest"))
    {
        auto& jm = j["manifest"];
        m.manifest_info.file_count = jm.value("file_count", 0);
        m.manifest_info.generated_utc = jm.value("generated_utc", "");
        if (jm.contains("entries"))
        {
            for (auto& e : jm["entries"])
            {
                std::pair<std::string, std::string> p;
                p.first = e.value("path", "");
                p.second = e.value("sha256", "");
                m.manifest_info.entries.push_back(p);
            }
        }
    }

    // Layers
    if (j.contains("layers"))
    {
        for (auto& jl : j["layers"])
        {
            ManifestLayer L;
            L.id = jl.value("id", "");
            L.name = jl.value("name", "");
            L.type = jl.value("type", "raster");
            L.visible = jl.value("visible", true);
            L.locked = jl.value("locked", false);
            L.opacity = jl.value("opacity", 1.0f);
            L.blend_mode = jl.value("blend_mode", "normal");
            L.path = jl.value("path", "");
            L.sha256 = jl.value("sha256", "");

            if (jl.contains("transform"))
            {
                auto& t = jl["transform"];
                L.transform.tx = t.value("tx", L.transform.tx);
                L.transform.ty = t.value("ty", L.transform.ty);
                L.transform.scaleX = t.value("scaleX", L.transform.scaleX);
                L.transform.scaleY = t.value("scaleY", L.transform.scaleY);
                L.transform.rotation = t.value("rotation", L.transform.rotation);
                L.transform.skewX = t.value("skewX", L.transform.skewX);
                L.transform.skewY = t.value("skewY", L.transform.skewY);
            }

            if (jl.contains("bounds"))
            {
                auto& b = jl["bounds"];
                L.bounds.x = b.value("x", L.bounds.x);
                L.bounds.y = b.value("y", L.bounds.y);
                L.bounds.width = b.value("width", L.bounds.width);
                L.bounds.height = b.value("height", L.bounds.height);
            }

            if (jl.contains("text_data") && L.type == "text")
            {
                TextData td;
                auto& jt = jl["text_data"];
                td.content = jt.value("content", td.content);
                td.font_family = jt.value("font_family", td.font_family);
                td.font_size = jt.value("font_size", td.font_size);
                td.font_weight = jt.value("font_weight", td.font_weight);
                td.alignment = jt.value("alignment", td.alignment);
                if (jt.contains("color"))
                {
                    auto& c = jt["color"];
                    td.color.r = c.value("r", td.color.r);
                    td.color.g = c.value("g", td.color.g);
                    td.color.b = c.value("b", td.color.b);
                    td.color.a = c.value("a", td.color.a);
                }
                L.text_data = td;
            }

            if (L.id.empty())
                warnings.push_back("Layer sans ID détecté");

            m.layers.push_back(L);
        }
    }

    // Layer groups
    if (j.contains("layer_groups"))
    {
        for (auto& jg : j["layer_groups"])
        {
            LayerGroup g;
            g.id = jg.value("id", "");
            g.name = jg.value("name", "");
            g.visible = jg.value("visible", true);
            g.locked = jg.value("locked", false);
            g.opacity = jg.value("opacity", 1.0f);
            g.blend_mode = jg.value("blend_mode", "normal");

            if (jg.contains("layer_ids"))
            {
                for (auto& lid : jg["layer_ids"])
                    g.layer_ids.push_back(lid.get<std::string>());
            }

            m.layer_groups.push_back(g);
        }
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

// ----------------- sérialisation JSON (retourne json* via void*) -----------

void* ZipEpgStorage::serializeCanvas(const Canvas& c) const
{
    json* j = new json;
    (*j)["name"] = c.name;
    (*j)["width"] = c.width;
    (*j)["height"] = c.height;
    (*j)["dpi"] = c.dpi;
    (*j)["color_space"] = c.color_space;
    (*j)["background"] = {
        {"r", c.background.r}, {"g", c.background.g}, {"b", c.background.b}, {"a", c.background.a}};
    return reinterpret_cast<void*>(j);
}

void* ZipEpgStorage::serializeLayer(const ManifestLayer& L) const
{
    json* j = new json;
    (*j)["id"] = L.id;
    (*j)["name"] = L.name;
    (*j)["type"] = L.type;
    (*j)["visible"] = L.visible;
    (*j)["locked"] = L.locked;
    (*j)["opacity"] = L.opacity;
    (*j)["blend_mode"] = L.blend_mode;
    (*j)["path"] = L.path;
    if (!L.sha256.empty())
        (*j)["sha256"] = L.sha256;

    (*j)["transform"] = {{"tx", L.transform.tx},
                         {"ty", L.transform.ty},
                         {"scaleX", L.transform.scaleX},
                         {"scaleY", L.transform.scaleY},
                         {"rotation", L.transform.rotation},
                         {"skewX", L.transform.skewX},
                         {"skewY", L.transform.skewY}};

    (*j)["bounds"] = {{"x", L.bounds.x},
                      {"y", L.bounds.y},
                      {"width", L.bounds.width},
                      {"height", L.bounds.height}};

    if (L.text_data.has_value())
    {
        auto& td = L.text_data.value();
        (*j)["text_data"] = {
            {"content", td.content},
            {"font_family", td.font_family},
            {"font_size", td.font_size},
            {"font_weight", td.font_weight},
            {"alignment", td.alignment},
            {"color",
             {{"r", td.color.r}, {"g", td.color.g}, {"b", td.color.b}, {"a", td.color.a}}}};
    }

    return reinterpret_cast<void*>(j);
}

void* ZipEpgStorage::serializeMetadata(const Metadata& m) const
{
    json* j = new json;
    (*j)["created_utc"] = m.created_utc;
    (*j)["modified_utc"] = m.modified_utc;
    if (!m.author.empty())
        (*j)["author"] = m.author;
    if (!m.description.empty())
        (*j)["description"] = m.description;
    if (!m.tags.empty())
        (*j)["tags"] = m.tags;
    if (!m.license.empty())
        (*j)["license"] = m.license;
    return reinterpret_cast<void*>(j);
}

// ----------------- conversion Document <-> Manifest -----------------------

ZipEpgStorage::Manifest ZipEpgStorage::createManifestFromDocument(const Document& doc) const
{
    Manifest m;
    m.epg_version = 1;

    // Canvas
    m.canvas.name = "EpiGimp2.0";
    m.canvas.width = doc.width;
    m.canvas.height = doc.height;
    m.canvas.dpi = static_cast<int>(doc.dpi);
    m.canvas.color_space = "sRGB";
    m.canvas.background = Color{255, 255, 255, 0};

    // IO defaults
    m.io.pixel_format_storage = "RGBA8_unorm_straight";
    m.io.pixel_format_runtime = "ARGB32_premultiplied";
    m.io.color_depth = 8;
    m.io.compression = "png";

    // Metadata
    std::string now = getCurrentTimestampUTC();
    m.metadata.author = "EpiGimp User";
    m.metadata.description = "Document créé avec EpiGimp";
    m.metadata.created_utc = now;
    m.metadata.modified_utc = now;

    // Layers
    for (size_t i = 0; i < doc.layers.size(); ++i)
    {
        std::string layerId = formatLayerId(i);
        ManifestLayer L;
        L.id = layerId;
        L.name = doc.layers[i]->name;
        L.type = "raster";
        L.visible = doc.layers[i]->visible;
        L.locked = doc.layers[i]->locked;
        L.opacity = doc.layers[i]->opacity;
        L.blend_mode = "normal";
        L.path = "layers/" + layerId + ".png";
        L.sha256 = "";  // sera remplie lors de la sauvegarde
        // transform default
        L.transform = Transform{};
        // bounds default = whole canvas
        L.bounds.x = 0;
        L.bounds.y = 0;
        L.bounds.width = doc.width;
        L.bounds.height = doc.height;
        m.layers.push_back(L);
    }

    return m;
}

std::unique_ptr<Document> ZipEpgStorage::createDocumentFromManifest(const Manifest& m,
                                                                    void* handle) const
{
    auto doc = std::make_unique<Document>();
    doc->width = m.canvas.width;
    doc->height = m.canvas.height;
    doc->dpi = static_cast<float>(m.canvas.dpi);

    // Charger chaque layer
    for (const auto& lm : m.layers)
    {
        try
        {
            // Lire les bytes PNG depuis le zip (raw)
            std::string pngData = readFileFromZip(handle, lm.path);

            // Si sha256 présent, vérifier
            if (!lm.sha256.empty())
            {
                if (!verifySHA256(pngData.data(), pngData.size(), lm.sha256))
                {
                    std::cerr << "Warning: checksum SHA256 mismatch for " << lm.path << std::endl;
                }
            }

            // Décoder
            auto buf = decodePngToImageBuffer(pngData);

            // Construire Layer et l'ajouter
            auto layer = std::make_shared<Layer>(std::stoull(lm.id));
            layer->name = lm.name;
            layer->visible = lm.visible;
            layer->locked = lm.locked;
            layer->opacity = lm.opacity;
            layer->pixels = std::move(buf);

            doc->layers.push_back(layer);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Avertissement: impossible de charger le layer " << lm.name << ": "
                      << e.what() << std::endl;
        }
    }

    return doc;
}

// ----------------- Chargement / sauvegarde PNG ---------------------------

std::unique_ptr<ImageBuffer> ZipEpgStorage::loadLayerFromZip(void* handle,
                                                             const std::string& path) const
{
    // Cette méthode est conservée pour compatibilité : lecture + décodage
    std::string pngData = readFileFromZip(handle, path);
    return decodePngToImageBuffer(pngData);
}

void ZipEpgStorage::saveLayerToZip(void* handle, const ImageBuffer& img,
                                   const std::string& path) const
{
    std::vector<unsigned char> pngData;

    stbi_write_png_to_func(pngWriteCallback, &pngData, img.width, img.height, 4, img.rgba.data(),
                           img.stride);

    if (pngData.empty())
        throw std::runtime_error("Failed to encode PNG for " + path);

    writeFileToZip(handle, path, pngData.data(), pngData.size());
}

void ZipEpgStorage::generatePreview(const Document& doc, void* handle) const
{
    if (doc.layers.empty())
        return;

    const int previewMax = 256;

    int pW = std::min(previewMax, doc.width);
    int pH = std::min(previewMax, doc.height);

    float scaleX = float(pW) / doc.width;
    float scaleY = float(pH) / doc.height;
    float scale = std::min(scaleX, scaleY);

    int w = std::max(1, int(doc.width * scale));
    int h = std::max(1, int(doc.height * scale));

    std::vector<unsigned char> preview(w * h * 4, 255);

    // (Tu pourras améliorer la compositing plus tard)

    std::vector<unsigned char> pngData;
    stbi_write_png_to_func(pngWriteCallback, &pngData, w, h, 4, preview.data(), w * 4);

    if (!pngData.empty())
        writeFileToZip(handle, "preview.png", pngData.data(), pngData.size());
}

// ----------------- OPEN --------------------------------------------------

OpenResult ZipEpgStorage::open(const std::string& path)
{
    int err = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_RDONLY, &err);
    if (!zip)
    {
        return {false, "Impossible d'ouvrir le ZIP (code: " + std::to_string(err) + ")", nullptr};
    }

    OpenResult res;
    try
    {
        std::string projectJson = readFileFromZip(zip, "project.json");
        std::vector<std::string> warnings;
        Manifest manifest = parseManifest(projectJson, warnings);
        validateManifest(manifest);

        res.document = createDocumentFromManifest(manifest, zip);
        res.success = true;

        for (auto& w : warnings)
            std::cout << "Warning: " << w << std::endl;
    }
    catch (const std::exception& e)
    {
        res.success = false;
        res.errorMessage = e.what();
    }

    zip_close(zip);
    return res;
}

// ----------------- SAVE --------------------------------------------------

void ZipEpgStorage::save(const Document& doc, const std::string& path)
{
    int err = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_TRUNCATE | ZIP_CREATE, &err);
    if (!zip)
        throw std::runtime_error("Impossible de créer le ZIP (code: " + std::to_string(err) + ")");

    try
    {
        Manifest m = createManifestFromDocument(doc);

        // Ecrire chaque calque PNG et calculer SHA256
        json layersJson = json::array();
        std::vector<std::pair<std::string, std::string>> manifestEntries;

        for (size_t i = 0; i < m.layers.size(); ++i)
        {
            const auto& L = m.layers[i];
            // Récupérer ImageBuffer depuis doc
            if (i >= doc.layers.size())
                throw std::runtime_error(
                    "Incohérence: nombre de calques différent entre Document et Manifest");

            auto& layerPtr = doc.layers[i];
            if (!layerPtr->pixels)
                throw std::runtime_error("Layer " + layerPtr->name + " n'a pas de pixels");

            // Encoder PNG en mémoire
            std::vector<unsigned char> buffer;
            int success = stbi_write_png_to_func(
                pngWriteCallback, &buffer, static_cast<int>(layerPtr->pixels->width),
                static_cast<int>(layerPtr->pixels->height),
                4,  // RGBA
                layerPtr->pixels->rgba.data(), static_cast<int>(layerPtr->pixels->stride));

            if (!success)
                throw std::runtime_error("Impossible d'encoder le PNG pour le layer " + L.path);

            // write accumulated buffer to zip
            writeFileToZip(zip, L.path, buffer.data(), buffer.size());

            // Calcul SHA256
            std::string sha = computeSHA256(buffer.data(), buffer.size());

            // Ecrire dans le zip
            writeFileToZip(zip, L.path, buffer.data(), buffer.size());


            // remplir layer JSON (avec sha)
            ManifestLayer layerWithSha = L;
            layerWithSha.sha256 = sha;
            json* lj = reinterpret_cast<json*>(serializeLayer(layerWithSha));
            layersJson.push_back(*lj);
            delete lj;

            // remplir entries pour manifest
            manifestEntries.emplace_back(L.path, sha);
        }

        // canvas JSON
        json* canvasJson = reinterpret_cast<json*>(serializeCanvas(m.canvas));
        json j;
        j["epg_version"] = m.epg_version;
        j["canvas"] = *canvasJson;
        delete canvasJson;

        // layers
        j["layers"] = layersJson;

        // layer_groups (vide pour l'instant)
        j["layer_groups"] = json::array();

        // io
        j["io"] = {{"pixel_format_storage", m.io.pixel_format_storage},
                   {"pixel_format_runtime", m.io.pixel_format_runtime},
                   {"color_depth", m.io.color_depth},
                   {"compression", m.io.compression}};

        // metadata
        m.metadata.modified_utc = getCurrentTimestampUTC();
        json* metaJson = reinterpret_cast<json*>(serializeMetadata(m.metadata));
        j["metadata"] = *metaJson;
        delete metaJson;

        // manifest (entries + file_count + generated_utc)
        json jManifest;
        jManifest["entries"] = json::array();
        for (const auto& e : manifestEntries)
        {
            json je;
            je["path"] = e.first;
            je["sha256"] = e.second;
            jManifest["entries"].push_back(je);
        }
        jManifest["file_count"] =
            static_cast<int>(1 + manifestEntries.size());  // project.json + layers
        jManifest["generated_utc"] = getCurrentTimestampUTC();
        j["manifest"] = jManifest;

        // écrire project.json
        std::string pj = j.dump(4);
        writeFileToZip(zip, "project.json", pj.data(), pj.size());

        // générer preview.PNG
        generatePreview(const_cast<Document&>(doc),
                        zip);  // doc n'est pas modifié, cast juste pour signature

        zip_close(zip);
    }
    catch (...)
    {
        zip_close(zip);
        throw;
    }
}

// ----------------- EXPORT PNG --------------------------------------------

void ZipEpgStorage::exportPng(const Document& doc, const std::string& path)
{
    if (doc.layers.empty())
        throw std::runtime_error("Document vide, impossible d'exporter");

    std::vector<unsigned char> out(static_cast<size_t>(doc.width) * doc.height * 4, 0);

    // Composer de bas en haut
    for (const auto& layerPtr : doc.layers)
    {
        if (!layerPtr->visible || !layerPtr->pixels)
            continue;

        const ImageBuffer& img = *layerPtr->pixels;
        for (int y = 0; y < doc.height && y < img.height; ++y)
        {
            for (int x = 0; x < doc.width && x < img.width; ++x)
            {
                int dstIdx = (y * doc.width + x) * 4;
                int srcIdx = (y * img.width + x) * 4;

                float alpha = (img.rgba[srcIdx + 3] / 255.0f) * layerPtr->opacity;

                for (int c = 0; c < 3; ++c)
                {
                    float src = img.rgba[srcIdx + c];
                    float dst = out[dstIdx + c];
                    out[dstIdx + c] =
                        static_cast<unsigned char>(src * alpha + dst * (1.0f - alpha));
                }

                out[dstIdx + 3] = std::max<unsigned char>(
                    out[dstIdx + 3], static_cast<unsigned char>(alpha * 255.0f));
            }
        }
    }

    if (!stbi_write_png(path.c_str(), doc.width, doc.height, 4, out.data(), doc.width * 4))
        throw std::runtime_error("Impossible d'écrire le fichier PNG: " + path);
}

// ----------------- utilitaires demandés par le header ---------------------

std::string ZipEpgStorage::generateLayerFilename(size_t index) const
{
    return "layers/" + formatLayerId(index) + ".png";
}

std::string ZipEpgStorage::getCurrentUTCTimestamp() const
{
    return getCurrentTimestampUTC();
}

void ZipEpgStorage::validateCanvasDimensions(int width, int height) const
{
    if (width <= 0 || height <= 0)
        throw std::runtime_error("Canvas invalide (dimensions <= 0)");
    if (width > 65535 || height > 65535)
        throw std::runtime_error("Canvas trop grand (max 65535x65535)");
}

void ZipEpgStorage::validateLayerOpacity(float opacity) const
{
    if (opacity < 0.f || opacity > 1.f)
        throw std::runtime_error("Opacité de layer invalide");
}

void ZipEpgStorage::validateRotation(float /*rotation*/) const
{
    // no-op for now
}

// ----------------- parsers / serializers auxiliaires (stubs) --------------

ZipEpgStorage::Transform ZipEpgStorage::parseTransform(const void* /*jsonObject*/) const
{
    return Transform{};
}
ZipEpgStorage::Bounds ZipEpgStorage::parseBounds(const void* /*jsonObject*/) const
{
    return Bounds{};
}
ZipEpgStorage::TextData ZipEpgStorage::parseTextData(const void* /*jsonObject*/) const
{
    return TextData{};
}
Color ZipEpgStorage::parseColor(const void* /*jsonObject*/) const
{
    return Color{};
}
ZipEpgStorage::ManifestLayer ZipEpgStorage::parseLayer(const void* /*jsonObject*/) const
{
    return ManifestLayer{};
}
ZipEpgStorage::LayerGroup ZipEpgStorage::parseLayerGroup(const void* /*jsonObject*/) const
{
    return LayerGroup{};
}
ZipEpgStorage::Canvas ZipEpgStorage::parseCanvas(const void* /*jsonObject*/) const
{
    return Canvas{};
}
ZipEpgStorage::IOConfig ZipEpgStorage::parseIOConfig(const void* /*jsonObject*/) const
{
    return IOConfig{};
}
ZipEpgStorage::Metadata ZipEpgStorage::parseMetadata(const void* /*jsonObject*/) const
{
    return Metadata{};
}

void* ZipEpgStorage::serializeTransform(const Transform& /*t*/) const
{
    return nullptr;
}
void* ZipEpgStorage::serializeBounds(const Bounds& /*b*/) const
{
    return nullptr;
}
void* ZipEpgStorage::serializeTextData(const TextData& /*td*/) const
{
    return nullptr;
}
void* ZipEpgStorage::serializeColor(const Color& /*c*/) const
{
    return nullptr;
}
void* ZipEpgStorage::serializeLayerGroup(const LayerGroup& /*group*/) const
{
    return nullptr;
}
void* ZipEpgStorage::serializeIOConfig(const IOConfig& /*io*/) const
{
    return nullptr;
}
