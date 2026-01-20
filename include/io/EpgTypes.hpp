#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "io/EpgZip.hpp"

namespace io::epg
{

struct Color
{
    uint8_t r{255}, g{255}, b{255}, a{255};
};

struct Transform
{
    float tx{0.0f};
    float ty{0.0f};
    float scaleX{1.0f};
    float scaleY{1.0f};
    float rotation{0.0f};
    float skewX{0.0f};
    float skewY{0.0f};
};

struct Bounds
{
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

struct TextData
{
    std::string content;
    std::string fontFamily{"Arial"};
    int fontSize{12};
    std::string fontWeight{"normal"};
    Color color{0, 0, 0, 255};
    std::string alignment{"left"};
};

// Enums for layer type and blend mode
enum class LayerType
{
    Raster,
    Text,
    Unknown
};

enum class BlendMode
{
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    Unknown
};

struct ManifestLayer
{
    std::string id;
    std::string name;
    LayerType type{LayerType::Raster};
    bool visible{true};
    bool locked{false};
    float opacity{1.0f};
    BlendMode blendMode{BlendMode::Normal};
    Transform transform;
    Bounds bounds;
    std::string path;
    std::string sha256;
    std::optional<TextData> textData;
};

struct LayerGroup
{
    std::string id;
    std::string name;
    bool visible{true};
    bool locked{false};
    float opacity{1.0f};
    BlendMode blendMode{BlendMode::Normal};
    std::vector<std::string> layerIds;
};

struct IOConfig
{
    std::string pixelFormatStorage{"RGBA8_unorm_straight"};
    std::string pixelFormatRuntime{"ARGB32_premultiplied"};
    int colorDepth{8};
    std::string compression{"png"};
};

struct Metadata
{
    std::string createdUtc;
    std::string modifiedUtc;
    std::string author;
    std::string description;
    std::vector<std::string> tags;
    std::string license;
};

struct ManifestInfo
{
    std::vector<std::pair<std::string, std::string>> entries;
    int fileCount{0};
    std::string generatedUtc;
};

struct Canvas
{
    std::string name{"EpiGimp2.0"};
    int width{800};
    int height{600};
    float dpi{72};
    std::string colorSpace{"sRGB"};
    Color background{255, 255, 255, 0};
};

struct Manifest
{
    int epgVersion{1};
    ManifestInfo manifestInfo;
    Canvas canvas;
    std::vector<ManifestLayer> layers;
    std::vector<LayerGroup> layerGroups;
    IOConfig io;
    Metadata metadata;
};

}  // namespace io::epg
