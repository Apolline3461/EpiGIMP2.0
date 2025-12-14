// JSON (de)serialization helpers for EPG structures
#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

// This header expects the types (Color, ZipEpgStorage::*) to be declared
// before it is included (so include it after `EpgFormat.hpp`).

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

inline void to_json(json& j, const ZipEpgStorage::Transform& t)
{
    j = json{{"tx", t.tx},
             {"ty", t.ty},
             {"scaleX", t.scaleX},
             {"scaleY", t.scaleY},
             {"rotation", t.rotation},
             {"skewX", t.skewX},
             {"skewY", t.skewY}};
}

inline void from_json(const json& j, ZipEpgStorage::Transform& t)
{
    t.tx = j.value("tx", t.tx);
    t.ty = j.value("ty", t.ty);
    t.scaleX = j.value("scaleX", t.scaleX);
    t.scaleY = j.value("scaleY", t.scaleY);
    t.rotation = j.value("rotation", t.rotation);
    t.skewX = j.value("skewX", t.skewX);
    t.skewY = j.value("skewY", t.skewY);
}

inline void to_json(json& j, const ZipEpgStorage::Bounds& b)
{
    j = json{{"x", b.x}, {"y", b.y}, {"width", b.width}, {"height", b.height}};
}

inline void from_json(const json& j, ZipEpgStorage::Bounds& b)
{
    b.x = j.value("x", b.x);
    b.y = j.value("y", b.y);
    b.width = j.value("width", b.width);
    b.height = j.value("height", b.height);
}

inline void to_json(json& j, const ZipEpgStorage::TextData& td)
{
    j = json{{"content", td.content},     {"font_family", td.font_family},
             {"font_size", td.font_size}, {"font_weight", td.font_weight},
             {"alignment", td.alignment}, {"color", td.color}};
}

inline void from_json(const json& j, ZipEpgStorage::TextData& td)
{
    td.content = j.value("content", td.content);
    td.font_family = j.value("font_family", td.font_family);
    td.font_size = j.value("font_size", td.font_size);
    td.font_weight = j.value("font_weight", td.font_weight);
    td.alignment = j.value("alignment", td.alignment);
    if (j.contains("color"))
        td.color = j["color"].get<Color>();
}

inline void to_json(json& j, const ZipEpgStorage::ManifestLayer& L)
{
    j = json{{"id", L.id},
             {"name", L.name},
             {"type", L.type},
             {"visible", L.visible},
             {"locked", L.locked},
             {"opacity", L.opacity},
             {"blend_mode", L.blend_mode},
             {"path", L.path}};
    if (!L.sha256.empty())
        j["sha256"] = L.sha256;
    j["transform"] = L.transform;
    j["bounds"] = L.bounds;
    if (L.text_data.has_value())
        j["text_data"] = L.text_data.value();
}

inline void from_json(const json& j, ZipEpgStorage::ManifestLayer& L)
{
    L.id = j.value("id", L.id);
    L.name = j.value("name", L.name);
    L.type = j.value("type", L.type);
    L.visible = j.value("visible", L.visible);
    L.locked = j.value("locked", L.locked);
    L.opacity = j.value("opacity", L.opacity);
    L.blend_mode = j.value("blend_mode", L.blend_mode);
    L.path = j.value("path", L.path);
    L.sha256 = j.value("sha256", L.sha256);
    if (j.contains("transform"))
        L.transform = j["transform"].get<ZipEpgStorage::Transform>();
    if (j.contains("bounds"))
        L.bounds = j["bounds"].get<ZipEpgStorage::Bounds>();
    if (j.contains("text_data") && L.type == "text")
        L.text_data = j["text_data"].get<ZipEpgStorage::TextData>();
}

inline void to_json(json& j, const ZipEpgStorage::LayerGroup& g)
{
    j = json{{"id", g.id},
             {"name", g.name},
             {"visible", g.visible},
             {"locked", g.locked},
             {"opacity", g.opacity},
             {"blend_mode", g.blend_mode},
             {"layer_ids", g.layer_ids}};
}

inline void from_json(const json& j, ZipEpgStorage::LayerGroup& g)
{
    g.id = j.value("id", g.id);
    g.name = j.value("name", g.name);
    g.visible = j.value("visible", g.visible);
    g.locked = j.value("locked", g.locked);
    g.opacity = j.value("opacity", g.opacity);
    g.blend_mode = j.value("blend_mode", g.blend_mode);
    if (j.contains("layer_ids"))
        g.layer_ids = j["layer_ids"].get<std::vector<std::string>>();
}

inline void to_json(json& j, const ZipEpgStorage::IOConfig& io)
{
    j = json{{"pixel_format_storage", io.pixel_format_storage},
             {"pixel_format_runtime", io.pixel_format_runtime},
             {"color_depth", io.color_depth},
             {"compression", io.compression}};
}

inline void from_json(const json& j, ZipEpgStorage::IOConfig& io)
{
    io.pixel_format_storage = j.value("pixel_format_storage", io.pixel_format_storage);
    io.pixel_format_runtime = j.value("pixel_format_runtime", io.pixel_format_runtime);
    io.color_depth = j.value("color_depth", io.color_depth);
    io.compression = j.value("compression", io.compression);
}

inline void to_json(json& j, const ZipEpgStorage::Metadata& m)
{
    j = json{{"created_utc", m.created_utc}, {"modified_utc", m.modified_utc}};
    if (!m.author.empty())
        j["author"] = m.author;
    if (!m.description.empty())
        j["description"] = m.description;
    if (!m.tags.empty())
        j["tags"] = m.tags;
    if (!m.license.empty())
        j["license"] = m.license;
}

inline void from_json(const json& j, ZipEpgStorage::Metadata& m)
{
    m.created_utc = j.value("created_utc", m.created_utc);
    m.modified_utc = j.value("modified_utc", m.modified_utc);
    m.author = j.value("author", m.author);
    m.description = j.value("description", m.description);
    if (j.contains("tags"))
        m.tags = j["tags"].get<std::vector<std::string>>();
    m.license = j.value("license", m.license);
}

inline void to_json(json& j, const ZipEpgStorage::ManifestInfo& mi)
{
    j = json::object();
    j["entries"] = json::array();
    for (const auto& e : mi.entries)
    {
        j["entries"].push_back(json{{"path", e.first}, {"sha256", e.second}});
    }
    j["file_count"] = mi.file_count;
    j["generated_utc"] = mi.generated_utc;
}

inline void from_json(const json& j, ZipEpgStorage::ManifestInfo& mi)
{
    mi.file_count = j.value("file_count", mi.file_count);
    mi.generated_utc = j.value("generated_utc", mi.generated_utc);
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

inline void to_json(json& j, const ZipEpgStorage::Canvas& c)
{
    j = json{{"name", c.name},
             {"width", c.width},
             {"height", c.height},
             {"dpi", c.dpi},
             {"color_space", c.color_space},
             {"background", c.background}};
}

inline void from_json(const json& j, ZipEpgStorage::Canvas& c)
{
    c.name = j.value("name", c.name);
    c.width = j.value("width", c.width);
    c.height = j.value("height", c.height);
    c.dpi = j.value("dpi", c.dpi);
    c.color_space = j.value("color_space", c.color_space);
    if (j.contains("background"))
        c.background = j["background"].get<Color>();
}

inline void to_json(json& j, const ZipEpgStorage::Manifest& m)
{
    j = json::object();
    j["epg_version"] = m.epg_version;
    j["canvas"] = m.canvas;
    j["io"] = m.io;
    j["metadata"] = m.metadata;
    j["layers"] = json::array();
    for (const auto& L : m.layers)
        j["layers"].push_back(L);
    j["layer_groups"] = m.layer_groups;
    j["manifest"] = m.manifest_info;
}

inline void from_json(const json& j, ZipEpgStorage::Manifest& m)
{
    m.epg_version = j.value("epg_version", m.epg_version);
    if (j.contains("canvas"))
        m.canvas = j["canvas"].get<ZipEpgStorage::Canvas>();
    if (j.contains("io"))
        m.io = j["io"].get<ZipEpgStorage::IOConfig>();
    if (j.contains("metadata"))
        m.metadata = j["metadata"].get<ZipEpgStorage::Metadata>();
    if (j.contains("layers"))
    {
        m.layers.clear();
        for (const auto& jl : j["layers"])
            m.layers.push_back(jl.get<ZipEpgStorage::ManifestLayer>());
    }
    if (j.contains("layer_groups"))
        m.layer_groups = j["layer_groups"].get<std::vector<ZipEpgStorage::LayerGroup>>();
    if (j.contains("manifest"))
        m.manifest_info = j["manifest"].get<ZipEpgStorage::ManifestInfo>();
}
