#pragma once

#include "io/EpgTypes.hpp"
#include "io/EpgZip.hpp"

// JSON (de)serialization helpers for EPG structures
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
// This header is self-contained: it includes `io/EpgTypes.hpp` and provides
// `to_json`/`from_json` for the epg types.

namespace io::epg
{

// Conversion helpers between enums and strings for JSON
inline std::string layerTypeToString(LayerType t)
{
    switch (t)
    {
        case LayerType::Raster:
            return "raster";
        case LayerType::Text:
            return "text";
        default:
            return "unknown";
    }
}

inline LayerType layerTypeFromString(const std::string& s)
{
    if (s == "raster")
        return LayerType::Raster;
    if (s == "text")
        return LayerType::Text;
    return LayerType::Unknown;
}

inline std::string blendModeToString(BlendMode m)
{
    switch (m)
    {
        case BlendMode::Normal:
            return "normal";
        case BlendMode::Multiply:
            return "multiply";
        case BlendMode::Screen:
            return "screen";
        case BlendMode::Overlay:
            return "overlay";
        case BlendMode::Darken:
            return "darken";
        case BlendMode::Lighten:
            return "lighten";
        default:
            return "unknown";
    }
}

inline BlendMode blendModeFromString(const std::string& s)
{
    if (s == "normal")
        return BlendMode::Normal;
    if (s == "multiply")
        return BlendMode::Multiply;
    if (s == "screen")
        return BlendMode::Screen;
    if (s == "overlay")
        return BlendMode::Overlay;
    if (s == "darken")
        return BlendMode::Darken;
    if (s == "lighten")
        return BlendMode::Lighten;
    return BlendMode::Unknown;
}

inline void to_json(json& j, const LayerType& t)
{
    j = layerTypeToString(t);
}

inline void from_json(const json& j, LayerType& t)
{
    if (j.is_string())
        t = layerTypeFromString(j.get<std::string>());
    else
        t = LayerType::Unknown;
}

inline void to_json(json& j, const BlendMode& m)
{
    j = blendModeToString(m);
}

inline void from_json(const json& j, BlendMode& m)
{
    if (j.is_string())
        m = blendModeFromString(j.get<std::string>());
    else
        m = BlendMode::Unknown;
}

inline void to_json(json& j, const Color& c)
{
    j = json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
}

inline void from_json(const json& j, Color& c)
{
    c.r = j.value("r", c.r);
    c.g = j.value("g", c.g);
    c.b = j.value("b", c.b);
    c.a = j.value("a", c.a);
}

inline void to_json(json& j, const Transform& t)
{
    j = json{{"tx", t.tx},
             {"ty", t.ty},
             {"scaleX", t.scaleX},
             {"scaleY", t.scaleY},
             {"rotation", t.rotation},
             {"skewX", t.skewX},
             {"skewY", t.skewY}};
}

inline void from_json(const json& j, Transform& t)
{
    t.tx = j.value("tx", t.tx);
    t.ty = j.value("ty", t.ty);
    t.scaleX = j.value("scaleX", t.scaleX);
    t.scaleY = j.value("scaleY", t.scaleY);
    t.rotation = j.value("rotation", t.rotation);
    t.skewX = j.value("skewX", t.skewX);
    t.skewY = j.value("skewY", t.skewY);
}

inline void to_json(json& j, const Bounds& b)
{
    j = json{{"x", b.x}, {"y", b.y}, {"width", b.width}, {"height", b.height}};
}

inline void from_json(const json& j, Bounds& b)
{
    b.x = j.value("x", b.x);
    b.y = j.value("y", b.y);
    b.width = j.value("width", b.width);
    b.height = j.value("height", b.height);
}

inline void to_json(json& j, const TextData& td)
{
    j = json{{"content", td.content},     {"fontFamily", td.fontFamily},
             {"fontSize", td.fontSize},   {"fontWeight", td.fontWeight},
             {"alignment", td.alignment}, {"color", td.color}};
}

inline void from_json(const json& j, TextData& td)
{
    td.content = j.value("content", td.content);
    td.fontFamily = j.value("fontFamily", td.fontFamily);
    td.fontSize = j.value("fontSize", td.fontSize);
    td.fontWeight = j.value("fontWeight", td.fontWeight);
    td.alignment = j.value("alignment", td.alignment);
    if (j.contains("color"))
        td.color = j["color"].get<Color>();
}

inline void to_json(json& j, const ManifestLayer& L)
{
    j = json{{"id", L.id},
             {"name", L.name},
             {"type", L.type},
             {"visible", L.visible},
             {"locked", L.locked},
             {"opacity", L.opacity},
             {"blendMode", L.blendMode},
             {"path", L.path}};
    if (!L.sha256.empty())
        j["sha256"] = L.sha256;
    j["transform"] = L.transform;
    j["bounds"] = L.bounds;
    if (L.textData.has_value())
        j["textData"] = L.textData.value();
}

inline void from_json(const json& j, ManifestLayer& L)
{
    L.id = j.value("id", L.id);
    L.name = j.value("name", L.name);
    L.type = j.value("type", L.type);
    L.visible = j.value("visible", L.visible);
    L.locked = j.value("locked", L.locked);
    L.opacity = j.value("opacity", L.opacity);
    L.blendMode = j.value("blendMode", L.blendMode);
    L.path = j.value("path", L.path);
    L.sha256 = j.value("sha256", L.sha256);
    if (j.contains("transform"))
        L.transform = j["transform"].get<Transform>();
    if (j.contains("bounds"))
        L.bounds = j["bounds"].get<Bounds>();
    if (j.contains("textData") && L.type == LayerType::Text)
        L.textData = j["textData"].get<TextData>();
}

inline void to_json(json& j, const LayerGroup& g)
{
    j = json{{"id", g.id},
             {"name", g.name},
             {"visible", g.visible},
             {"locked", g.locked},
             {"opacity", g.opacity},
             {"blendMode", g.blendMode},
             {"layerIds", g.layerIds}};
}

inline void from_json(const json& j, LayerGroup& g)
{
    g.id = j.value("id", g.id);
    g.name = j.value("name", g.name);
    g.visible = j.value("visible", g.visible);
    g.locked = j.value("locked", g.locked);
    g.opacity = j.value("opacity", g.opacity);
    g.blendMode = j.value("blendMode", g.blendMode);
    if (j.contains("layerIds"))
        g.layerIds = j["layerIds"].get<std::vector<std::string>>();
}

inline void to_json(json& j, const IOConfig& io)
{
    j = json{{"pixelFormatStorage", io.pixelFormatStorage},
             {"pixelFormatRuntime", io.pixelFormatRuntime},
             {"colorDepth", io.colorDepth},
             {"compression", io.compression}};
}

inline void from_json(const json& j, IOConfig& io)
{
    io.pixelFormatStorage = j.value("pixelFormatStorage", io.pixelFormatStorage);
    io.pixelFormatRuntime = j.value("pixelFormatRuntime", io.pixelFormatRuntime);
    io.colorDepth = j.value("colorDepth", io.colorDepth);
    io.compression = j.value("compression", io.compression);
}

inline void to_json(json& j, const Metadata& m)
{
    j = json{{"createdUtc", m.createdUtc}, {"modifiedUtc", m.modifiedUtc}};
    if (!m.author.empty())
        j["author"] = m.author;
    if (!m.description.empty())
        j["description"] = m.description;
    if (!m.tags.empty())
        j["tags"] = m.tags;
    if (!m.license.empty())
        j["license"] = m.license;
}

inline void from_json(const json& j, Metadata& m)
{
    m.createdUtc = j.value("createdUtc", m.createdUtc);
    m.modifiedUtc = j.value("modifiedUtc", m.modifiedUtc);
    m.author = j.value("author", m.author);
    m.description = j.value("description", m.description);
    if (j.contains("tags"))
        m.tags = j["tags"].get<std::vector<std::string>>();
    m.license = j.value("license", m.license);
}

inline void to_json(json& j, const ManifestInfo& mi)
{
    j = json::object();
    j["entries"] = json::array();
    for (const auto& e : mi.entries)
    {
        j["entries"].push_back(json{{"path", e.first}, {"sha256", e.second}});
    }
    j["fileCount"] = mi.fileCount;
    j["generatedUtc"] = mi.generatedUtc;
}

inline void from_json(const json& j, ManifestInfo& mi)
{
    mi.fileCount = j.value("fileCount", mi.fileCount);
    mi.generatedUtc = j.value("generatedUtc", mi.generatedUtc);
    if (j.contains("entries"))
    {
        mi.entries.clear();
        for (const auto& e : j["entries"])
        {
            std::string path = e.value("path", std::string());
            std::string sha = e.value("sha256", std::string());
            mi.entries.emplace_back(path, sha);
        }
    }
}

inline void to_json(json& j, const Canvas& c)
{
    j = json{{"name", c.name}, {"width", c.width},           {"height", c.height},
             {"dpi", c.dpi},   {"colorSpace", c.colorSpace}, {"background", c.background}};
}

inline void from_json(const json& j, Canvas& c)
{
    c.name = j.value("name", c.name);
    c.width = j.value("width", c.width);
    c.height = j.value("height", c.height);
    c.dpi = j.value("dpi", c.dpi);
    c.colorSpace = j.value("colorSpace", c.colorSpace);
    if (j.contains("background"))
        c.background = j["background"].get<Color>();
}

inline void to_json(json& j, const Manifest& m)
{
    j = json::object();
    j["epgVersion"] = m.epgVersion;
    j["canvas"] = m.canvas;
    j["io"] = m.io;
    j["metadata"] = m.metadata;
    j["layers"] = json::array();
    for (const auto& L : m.layers)
        j["layers"].push_back(L);
    j["layerGroups"] = m.layerGroups;
    j["manifestInfo"] = m.manifestInfo;
}

inline void from_json(const json& j, Manifest& m)
{
    m.epgVersion = j.value("epgVersion", m.epgVersion);
    if (j.contains("canvas"))
        m.canvas = j["canvas"].get<Canvas>();
    if (j.contains("io"))
        m.io = j["io"].get<IOConfig>();
    if (j.contains("metadata"))
        m.metadata = j["metadata"].get<Metadata>();
    if (j.contains("layers"))
    {
        m.layers.clear();
        for (const auto& jl : j["layers"])
            m.layers.push_back(jl.get<ManifestLayer>());
    }
    if (j.contains("layerGroups"))
        m.layerGroups = j["layerGroups"].get<std::vector<LayerGroup>>();
    if (j.contains("manifestInfo"))
        m.manifestInfo = j["manifestInfo"].get<ManifestInfo>();
}

}  // namespace io::epg
