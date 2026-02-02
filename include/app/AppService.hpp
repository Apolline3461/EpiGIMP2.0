//
// Created by apolline on 21/01/2026.
//

#pragma once

#include "app/History.hpp"
#include "app/Signal.h"
#include "io/IStorage.hpp"

class Document;
namespace app
{
struct Size
{
    int w;
    int h;
};

struct LayerSpec
{  // maybe we can use it for other things in that's case we will put in a separated file
    std::uint32_t color = 0xFFFFFFFFU;
    std::string name = "Layer ";
    bool visible = true;
    bool locked = true;
    float opacity = 1.F;
};

class AppService
{
   public:
    explicit AppService(std::unique_ptr<IStorage> storage);
    ~AppService() = default;

    void newDocument(Size size, float dpi);
    void open(const std::string& path);
    void save(const std::string& path);
    void exportPng(const std::string& path);

    const Document& document() const;

    std::size_t activeLayer() const;
    void setActiveLayer(std::size_t idx);

    void addLayer(const LayerSpec& spec);
    void removeLayer(std::size_t idx);
    void setLayerLocked(std::size_t idx, bool locked);

    void undo();
    void redo();
    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

    Signal documentChanged;

   private:
    std::unique_ptr<IStorage> storage_;
    app::History history_;
    std::unique_ptr<Document> doc_;
    std::size_t activeLayer_;
    std::uint64_t nextLayerId_ = 1;
};
};  // namespace app
