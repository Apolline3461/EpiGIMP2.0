//
// Created by apolline on 21/01/2026.
//

#pragma once

#include "app/History.hpp"
#include "app/Signal.h"
#include "core/Document.hpp"
#include "io/IStorage.hpp"

class IStorage;
class Document;
namespace app
{
struct Size
{
    int w;
    int h;
};

class AppService
{
   public:
    explicit AppService(std::unique_ptr<IStorage> storage);

    void newDocument(Size size, float dpi);
    void open(const std::string& path);
    void save(const std::string& path);
    void exportPng(const std::string& path);

    const Document& document() const;

    std::size_t activeLayer() const;
    void setActiveLayer(std::size_t idx);

    void addLayer();
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
    Document doc_;
    std::size_t activeLayer_{0};
};
};  // namespace app
