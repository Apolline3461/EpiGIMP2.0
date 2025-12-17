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
    std::string font_family{"Arial"};
    int font_size{12};
    std::string font_weight{"normal"};
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
    BlendMode blend_mode{BlendMode::Normal};
    Transform transform;
    Bounds bounds;
    std::string path;
    std::string sha256;
    std::optional<TextData> text_data;
};

struct LayerGroup
{
    std::string id;
    std::string name;
    bool visible{true};
    bool locked{false};
    float opacity{1.0f};
    BlendMode blend_mode{BlendMode::Normal};
    std::vector<std::string> layer_ids;
};

struct IOConfig
{
    std::string pixel_format_storage{"RGBA8_unorm_straight"};
    std::string pixel_format_runtime{"ARGB32_premultiplied"};
    int color_depth{8};
    std::string compression{"png"};
};

struct Metadata
{
    std::string created_utc;
    std::string modified_utc;
    std::string author;
    std::string description;
    std::vector<std::string> tags;
    std::string license;
};

struct ManifestInfo
{
    std::vector<std::pair<std::string, std::string>> entries;
    int file_count{0};
    std::string generated_utc;
};

struct Canvas
{
    std::string name{"EpiGimp2.0"};
    int width{800};
    int height{600};
    int dpi{72};
    std::string color_space{"sRGB"};
    Color background{255, 255, 255, 0};
};

struct Manifest
{
    int epg_version{1};
    ManifestInfo manifest_info;
    Canvas canvas;
    std::vector<ManifestLayer> layers;
    std::vector<LayerGroup> layer_groups;
    IOConfig io;
    Metadata metadata;
};

}  // namespace io::epg
