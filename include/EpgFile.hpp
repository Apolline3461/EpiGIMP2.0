#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Forward declarations
class Document;
class ImageBuffer;

struct Color
{
    uint8_t r{255}, g{255}, b{255}, a{255};
};

struct OpenResult
{
    bool success{false};
    std::string errorMessage;
    std::unique_ptr<Document> document;
};

class ZipEpgStorage
{
   public:
    // Structure pour les transformations 2D
    struct Transform
    {
        float tx{0.0f};        // Translation X
        float ty{0.0f};        // Translation Y
        float scaleX{1.0f};    // Échelle X
        float scaleY{1.0f};    // Échelle Y
        float rotation{0.0f};  // Rotation en degrés
        float skewX{0.0f};     // Inclinaison X (optionnel)
        float skewY{0.0f};     // Inclinaison Y (optionnel)
    };

    // Structure pour les rectangles englobants
    struct Bounds
    {
        int x{0};
        int y{0};
        int width{0};
        int height{0};
    };

    // Structure pour les données de texte
    struct TextData
    {
        std::string content;
        std::string font_family{"Arial"};
        int font_size{12};
        std::string font_weight{"normal"};  // normal, bold, light
        Color color{0, 0, 0, 255};
        std::string alignment{"left"};  // left, center, right, justify
    };

    // Structure pour un calque dans le manifest
    struct ManifestLayer
    {
        std::string id;  // Format "NNNN"
        std::string name;
        std::string type{"raster"};  // raster, text, shape, adjustment
        bool visible{true};
        bool locked{false};
        float opacity{1.0f};
        std::string blend_mode{"normal"};  // normal, multiply
        Transform transform;
        Bounds bounds;
        std::string path;    // Chemin relatif (ex: "layers/0001.png")
        std::string sha256;  // Hash SHA-256 du fichier

        // Données spécifiques au type
        std::optional<TextData> text_data;  // Uniquement pour type="text"
    };

    // Structure pour les groupes de calques
    struct LayerGroup
    {
        std::string id;
        std::string name;
        bool visible{true};
        bool locked{false};
        float opacity{1.0f};
        std::string blend_mode{"normal"};
        std::vector<std::string> layer_ids;  // IDs des calques membres
    };

    // Structure pour la sélection active
    struct Selection
    {
        bool active{false};
        std::string type;  // rect, ellipse, path, etc.
        std::optional<Bounds> bounds;
        std::string path;  // Chemin vers fichier SVG si sélection path
    };

    // Structure pour la configuration I/O
    struct IOConfig
    {
        std::string pixel_format_storage{"RGBA8_unorm_straight"};
        std::string pixel_format_runtime{"ARGB32_premultiplied"};
        int color_depth{8};
        std::string compression{"png"};
    };

    // Structure pour les métadonnées
    struct Metadata
    {
        std::string created_utc;
        std::string modified_utc;
        std::string author;
        std::string description;
        std::vector<std::string> tags;
        std::string license;
    };

    // Structure pour le manifeste d'intégrité
    struct ManifestInfo
    {
        std::vector<std::pair<std::string, std::string>> entries;  // <path, sha256>
        int file_count{0};
        std::string generated_utc;
    };

    // Structure pour le canvas
    struct Canvas
    {
        std::string name{"EpiGimp2.0"};
        int width{800};
        int height{600};
        int dpi{72};
        std::string color_space{"sRGB"};  // sRGB, Adobe RGB, ProPhoto RGB
        Color background{255, 255, 255, 0};
    };

    // Structure principale du manifest (project.json)
    struct Manifest
    {
        int epg_version{1};
        ManifestInfo manifest_info;
        Canvas canvas;
        std::vector<ManifestLayer> layers;
        std::vector<LayerGroup> layer_groups;
        IOConfig io;
        Metadata metadata;
        Selection selection;
    };

    ZipEpgStorage() = default;
    ~ZipEpgStorage() = default;

    // Interface IStorage
    OpenResult open(const std::string& path);
    void save(const Document& doc, const std::string& path);
    void exportPng(const Document& doc, const std::string& path);

   private:
    // Parsing et validation du manifest
    Manifest parseManifest(const std::string& jsonText, std::vector<std::string>& warnings) const;
    void validateManifest(const Manifest& m) const;

    // Parsing des sous-structures JSON
    Transform parseTransform(const void* jsonObject) const;
    Bounds parseBounds(const void* jsonObject) const;
    TextData parseTextData(const void* jsonObject) const;
    Color parseColor(const void* jsonObject) const;
    ManifestLayer parseLayer(const void* jsonObject) const;
    LayerGroup parseLayerGroup(const void* jsonObject) const;
    Canvas parseCanvas(const void* jsonObject) const;
    IOConfig parseIOConfig(const void* jsonObject) const;
    Metadata parseMetadata(const void* jsonObject) const;
    Selection parseSelection(const void* jsonObject) const;

    // Sérialisation vers JSON
    void* serializeTransform(const Transform& t) const;
    void* serializeBounds(const Bounds& b) const;
    void* serializeTextData(const TextData& td) const;
    void* serializeColor(const Color& c) const;
    void* serializeLayer(const ManifestLayer& layer) const;
    void* serializeLayerGroup(const LayerGroup& group) const;
    void* serializeCanvas(const Canvas& canvas) const;
    void* serializeIOConfig(const IOConfig& io) const;
    void* serializeMetadata(const Metadata& meta) const;
    void* serializeSelection(const Selection& sel) const;

    // Méthodes utilitaires pour la manipulation du ZIP
    std::string readFileFromZip(void* zipHandle, const std::string& filename) const;
    void writeFileToZip(void* zipHandle, const std::string& filename, const void* data,
                        size_t size) const;

    // Génération et vérification de checksums
    std::string computeSHA256(const void* data, size_t size) const;
    bool verifySHA256(const void* data, size_t size, const std::string& expectedHash) const;

    // Conversion entre Document et Manifest
    Manifest createManifestFromDocument(const Document& doc) const;
    std::unique_ptr<Document> createDocumentFromManifest(const Manifest& manifest,
                                                         void* zipHandle) const;

    // Chargement/sauvegarde des calques et preview
    std::unique_ptr<ImageBuffer> loadLayerFromZip(void* zipHandle, const std::string& path) const;
    void saveLayerToZip(void* zipHandle, const ImageBuffer& layer, const std::string& path) const;
    void generatePreview(const Document& doc, void* zipHandle) const;

    // Génération de noms de fichiers et timestamps
    std::string generateLayerFilename(size_t index) const;
    std::string getCurrentUTCTimestamp() const;

    // Validation des limites
    void validateCanvasDimensions(int width, int height) const;
    void validateLayerOpacity(float opacity) const;
    void validateRotation(float rotation) const;
};

// Interface abstraite pour le stockage (polymorphisme)
class IStorage
{
   public:
    virtual ~IStorage() = default;
    virtual OpenResult open(const std::string& path) = 0;
    virtual void save(const Document& doc, const std::string& path) = 0;
    virtual void exportPng(const Document& doc, const std::string& path) = 0;
};