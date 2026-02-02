//
// Created by apolline on 21/01/2026.
//

#pragma once

#include "app/History.hpp"
#include "app/Signal.h"
#include "common/Geometry.hpp"
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

    const Document& document() const;

    void newDocument(Size size, float dpi);
    void open(const std::string& path);
    void save(const std::string& path);
    void exportImage(const std::string& path);

    std::size_t activeLayer() const;
    void setActiveLayer(std::size_t idx);
    void setLayerVisible(std::size_t idx, bool visible);
    void setLayerOpacity(std::size_t idx, float alpha);
    void setLayerLocked(std::size_t idx, bool locked);

    void addLayer(const LayerSpec& spec);
    void removeLayer(std::size_t idx);
    void reorderLayer(std::size_t from, std::size_t to);
    void mergeLayerDown(std::size_t from);

    // void beginStroke(const ToolParams&, Point pStart);
    // void moveStroke(Point p);
    // void endStroke();
    uint32_t pickColorAt(common::Point p) const;

    const Selection& selection() const;
    void setSelectionRect(common::Rect r);
    void clearSelectionRect();

    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const noexcept;
    [[nodiscard]] bool canRedo() const noexcept;

    Signal documentChanged;

   private:
    std::unique_ptr<IStorage> storage_;
    app::History history_ = app::History(20);
    std::unique_ptr<Document> doc_;
    std::size_t activeLayer_ = 0;
    std::uint64_t nextLayerId_ = 1;
    std::unique_ptr<Command> currentStroke_;
    // std::vector<common::PointPoint> pointToDraw_;
    // std::optional<common::Point> lastPointToDraw_;
    void apply(std::unique_ptr<Command> cmd);
};
};  // namespace app
