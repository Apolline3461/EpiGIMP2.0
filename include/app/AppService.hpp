//
// Created by apolline on 21/01/2026.
//

#pragma once
#include <memory>
#include <string>

#include "app/History.hpp"
#include "app/Signal.h"
#include "app/ToolParams.hpp"
#include "commands/StrokeCommand.hpp"
#include "common/Geometry.hpp"
#include "io/IStorage.hpp"

class Document;

namespace app
{
namespace commands
{
class StrokeCommand;

}

struct Size
{
    int w;
    int h;
};

struct LayerSpec  // TODO: put it in other file
{
    std::string name = "Layer ";
    bool visible = true;
    bool locked = false;
    float opacity = 1.F;
    std::uint32_t color = 0U;

    std::optional<size_t> width;
    std::optional<size_t> height;

    //positon on the doc
    int offsetX = 0;
    int offsetY = 0;
};

class AppService
{
   public:
    explicit AppService(std::unique_ptr<IStorage> storage);
    ~AppService() = default;

    const Document& document() const;
    Document& document();

    [[nodiscard]] bool hasDocument() const;
    void newDocument(Size size, float dpi, std::uint32_t bgColor = common::colors::White);
    void open(const std::string& path);
    void replaceBackgroundWithImage(const ImageBuffer& img, std::string name = "Background");
    void save(const std::string& path);
    void exportImage(const std::string& path);
    void closeDocument();

    std::size_t activeLayer() const;
    void setActiveLayer(std::size_t idx);
    void setLayerVisible(std::size_t idx, bool visible);
    void setLayerOpacity(std::size_t idx, float alpha);
    void setLayerLocked(std::size_t idx, bool locked);
    void setLayerName(std::size_t idx, std::string name);

    void addLayer(const LayerSpec& spec);
    void addImageLayer(const ImageBuffer& img, std::string name, bool visible = true,
                       bool locked = false, float opacity = 1.f);
    void removeLayer(std::size_t idx);
    void reorderLayer(std::size_t from, std::size_t to);
    void mergeLayerDown(std::size_t from);
    void moveLayer(std::size_t idx, int newOffsetX, int newOffsetY);

    void resizeLayer(std::size_t idx, int newW, int newH,
                     bool smooth = true);  // smooth true = bilinear, nearest

    void beginStroke(const ToolParams&, common::Point pStart);
    void moveStroke(common::Point p);
    void endStroke();
    uint32_t pickColorAt(common::Point p) const;

    const Selection& selection() const;
    void setSelectionRect(common::Rect r);
    void clearSelectionRect();

    void bucketFill(common::Point p, std::uint32_t rgba);

    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const noexcept;
    [[nodiscard]] bool canRedo() const noexcept;

    Signal documentChanged;

   private:
    std::unique_ptr<IStorage> storage_;
    History history_ = History(20);
    std::unique_ptr<Document> doc_;
    std::size_t activeLayer_ = 0;
    std::uint64_t nextLayerId_ = 1;
    std::unique_ptr<commands::StrokeCommand> currentStroke_;
    // std::vector<common::PointPoint> pointToDraw_;
    // std::optional<common::Point> lastPointToDraw_;
    void apply(std::unique_ptr<Command> cmd);
};
};  // namespace app
