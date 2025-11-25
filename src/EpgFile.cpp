#include <openssl/sha.h>
#include <zip.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "document.hpp"
#include "imagebuffer.hpp"
#include "EpgFile.hpp"

using json = nlohmann::json;

// ============================================================================
// Outils internes
// ============================================================================

static std::string loadEntireFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("Impossible d’ouvrir le fichier : " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void writeFile(const std::string& path, const std::string& data)
{
    std::ofstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("Impossible d’écrire dans : " + path);
    f.write(data.data(), data.size());
}

// ============================================================================
// ZIP read/write helpers
// ============================================================================

std::string ZipEpgStorage::readFileFromZip(void* handle, const std::string& filename) const
{
    zip_t* zip = reinterpret_cast<zip_t*>(handle);

    struct zip_stat st;
    zip_stat_init(&st);

    if (zip_stat(zip, filename.c_str(), 0, &st) != 0)
        throw std::runtime_error("Fichier introuvable dans le ZIP : " + filename);

    zip_file_t* zf = zip_fopen(zip, filename.c_str(), 0);
    if (!zf)
        throw std::runtime_error("Impossible d’ouvrir " + filename + " dans le ZIP");

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

    zip_source_t* source = zip_source_buffer(zip, data, size, 0);
    if (!source)
        throw std::runtime_error("zip_source_buffer failed : " + std::string(zip_strerror(zip)));

    if (zip_file_add(zip, filename.c_str(), source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0)
    {
        zip_source_free(source);
        throw std::runtime_error("Impossible d’ajouter le fichier dans le ZIP : " + filename);
    }
}

// ============================================================================
// SHA-256
// ============================================================================

std::string ZipEpgStorage::computeSHA256(const void* data, size_t size) const
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)data, size, hash);

    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

    return ss.str();
}

bool ZipEpgStorage::verifySHA256(const void* data, size_t size, const std::string& expected) const
{
    return computeSHA256(data, size) == expected;
}

// ============================================================================
// Parsing JSON → Manifest
// ============================================================================

ZipEpgStorage::Manifest ZipEpgStorage::parseManifest(const std::string& jsonText,
                                                     std::vector<std::string>&) const
{
    json j = json::parse(jsonText);

    Manifest m;
    m.epg_version = j.value("epg_version", 1);

    // Canvas
    m.canvas.width = j["canvas"].value("width", 800);
    m.canvas.height = j["canvas"].value("height", 600);
    m.canvas.name = j["canvas"].value("name", "EpiGimp2.0");

    // Layers
    for (auto& jl : j["layers"])
    {
        ManifestLayer L;
        L.id = jl.value("id", "");
        L.name = jl.value("name", "");
        L.type = jl.value("type", "raster");
        L.visible = jl.value("visible", true);
        L.opacity = jl.value("opacity", 1.0f);
        L.path = jl.value("path", "");

        m.layers.push_back(L);
    }

    return m;
}

void ZipEpgStorage::validateManifest(const Manifest& m) const
{
    if (m.canvas.width <= 0 || m.canvas.height <= 0)
        throw std::runtime_error("Canvas invalide (dimensions <= 0)");

    for (auto& l : m.layers)
    {
        if (l.opacity < 0.f || l.opacity > 1.f)
            throw std::runtime_error("Opacité invalide pour le layer : " + l.id);
    }
}

// ============================================================================
// Manifest → JSON
// ============================================================================

void* ZipEpgStorage::serializeCanvas(const Canvas& c) const
{
    json* j = new json;
    (*j)["name"] = c.name;
    (*j)["width"] = c.width;
    (*j)["height"] = c.height;
    (*j)["dpi"] = c.dpi;
    (*j)["color_space"] = c.color_space;
    return j;
}

void* ZipEpgStorage::serializeLayer(const ManifestLayer& L) const
{
    json* j = new json;
    (*j)["id"] = L.id;
    (*j)["name"] = L.name;
    (*j)["type"] = L.type;
    (*j)["visible"] = L.visible;
    (*j)["opacity"] = L.opacity;
    (*j)["path"] = L.path;
    return j;
}

void* ZipEpgStorage::serializeMetadata(const Metadata& m) const
{
    json* j = new json;
    (*j)["author"] = m.author;
    (*j)["description"] = m.description;
    (*j)["created_utc"] = m.created_utc;
    (*j)["modified_utc"] = m.modified_utc;
    return j;
}

void* ZipEpgStorage::serializeSelection(const Selection& s) const
{
    json* j = new json;
    (*j)["active"] = s.active;
    (*j)["type"] = s.type;
    return j;
}

// ============================================================================
// Conversion Document ↔ Manifest (à compléter selon ton application)
// ============================================================================

ZipEpgStorage::Manifest ZipEpgStorage::createManifestFromDocument(const Document&) const
{
    Manifest m;
    m.canvas.width = 800;
    m.canvas.height = 600;
    return m;
}

std::unique_ptr<Document> ZipEpgStorage::createDocumentFromManifest(const Manifest&, void*) const
{
    return std::make_unique<Document>();
}

// ============================================================================
// Chargement calque (stub)
// ============================================================================

std::unique_ptr<ImageBuffer> ZipEpgStorage::loadLayerFromZip(void*, const std::string&) const
{
    return std::make_unique<ImageBuffer>();
}

void ZipEpgStorage::saveLayerToZip(void*, const ImageBuffer&, const std::string&) const
{
    // Implémentation à adapter
}

void ZipEpgStorage::generatePreview(const Document&, void*) const
{
    // Implémentation optionnelle
}

// ============================================================================
// OPEN
// ============================================================================

OpenResult ZipEpgStorage::open(const std::string& path)
{
    int err = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_RDONLY, &err);
    if (!zip)
    {
        return {false, "Impossible d’ouvrir le ZIP", nullptr};
    }

    OpenResult result;

    try
    {
        // Lire project.json
        std::string jsonText = readFileFromZip(zip, "project.json");

        std::vector<std::string> warnings;
        Manifest manifest = parseManifest(jsonText, warnings);
        validateManifest(manifest);

        result.document = createDocumentFromManifest(manifest, zip);
        result.success = true;
    }
    catch (const std::exception& e)
    {
        result.success = false;
        result.errorMessage = e.what();
    }

    zip_close(zip);
    return result;
}

// ============================================================================
// SAVE
// ============================================================================

void ZipEpgStorage::save(const Document& doc, const std::string& path)
{
    int err = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_TRUNCATE | ZIP_CREATE, &err);
    if (!zip)
        throw std::runtime_error("Impossible de créer le ZIP");

    Manifest m = createManifestFromDocument(doc);

    json j;
    j["epg_version"] = m.epg_version;
    j["canvas"] = *reinterpret_cast<json*>(serializeCanvas(m.canvas));

    // Layers
    j["layers"] = json::array();
    for (auto& L : m.layers)
    {
        std::cout << "layer" << L.id << std::endl;
        j["layers"].push_back(*reinterpret_cast<json*>(serializeLayer(L)));
    }

    // Metadata
    j["metadata"] = *reinterpret_cast<json*>(serializeMetadata(m.metadata));

    // Selection
    j["selection"] = *reinterpret_cast<json*>(serializeSelection(m.selection));

    std::string jsonText = j.dump(4);
    writeFileToZip(zip, "project.json", jsonText.data(), jsonText.size());

    zip_close(zip);
}

// ============================================================================
// PNG EXPORT (simplifié)
// ============================================================================

void ZipEpgStorage::exportPng(const Document&, const std::string& path)
{
    // Ici tu mets ton vrai code d’export
    writeFile(path, "PNG OUTPUT PLACEHOLDER");
}
