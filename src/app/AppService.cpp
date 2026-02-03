//
// Created by apolline on 21/01/2026.
//
#include "app/AppService.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>

#include "app/Command.hpp"
#include "app/commands/CommandUtils.hpp"
#include "app/commands/LayerCommands.hpp"
#include "app/ToolParams.hpp"
#include "common/Colors.hpp"
#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

namespace app
{

static std::uint64_t computeNextLayerId(const Document& doc)
{
    std::uint64_t maxId = 0;
    for (size_t i = 0; i < doc.layerCount(); ++i)
    {
        if (auto layer = doc.layerAt(i))
        {
            maxId = std::max(maxId, layer->id());
        }
    }
    return maxId + 1;
}

AppService::AppService(std::unique_ptr<IStorage> storage) : storage_(std::move(storage)) {}

const Document& AppService::document() const
{
    return *doc_;
}

void AppService::newDocument(Size size, float dpi)
{
    doc_ = std::make_unique<Document>(size.w, size.h, dpi);
    history_.clear();
    activeLayer_ = 0;
    nextLayerId_ = 1;

    auto img = std::make_shared<ImageBuffer>(size.w, size.h);
    img->fill(0xFFFFFFFFu);  // blanc

    auto layer = std::make_shared<Layer>(
        /*id*/ 0,
        /*name*/ "Background",
        /*image*/ img,
        /*visible*/ true,
        /*locked*/ true,
        /*opacity*/ 1.f);
    doc_->addLayer(std::move(layer));

    documentChanged.emit();
}

void AppService::open(const std::string& path)
{
    if (!storage_)
        throw std::runtime_error("Failed Open: storage is null");
    auto result = storage_->open(path);
    if (!result.document)
        throw std::runtime_error("Failed Open: failed to load document");
    doc_ = std::move(result.document);
    history_.clear();
    activeLayer_ = 0;

    nextLayerId_ = computeNextLayerId(*doc_);
    documentChanged.emit();
}

void AppService::save(const std::string& path)
{
    if (!storage_)
        throw std::runtime_error("Failed Save: storage is null");
    if (!doc_)
        throw std::runtime_error("Failed Save: document is null");
    storage_->save(*doc_, path);
}

void AppService::exportImage(const std::string& path)
{
    if (!storage_)
        throw std::runtime_error("Failed Export: storage is null");
    if (!doc_)
        throw std::runtime_error("Failed Export: document is null");
    storage_->exportImage(*doc_, path);
}

std::size_t AppService::activeLayer() const
{
    return activeLayer_;
}

void AppService::setActiveLayer(const std::size_t idx)
{
    if (!doc_)
        throw std::runtime_error("setActiveLayer: document is null");

    const size_t count = doc_->layerCount();
    if (idx >= static_cast<std::size_t>(count))
    {
        throw std::out_of_range("From setActiveLayer: index out of range");
    }
    activeLayer_ = idx;
}

void AppService::setLayerVisible(std::size_t idx, bool visible)
{
    if (!doc_)
        throw std::runtime_error("setLayerVisible: document is null");

    auto layer = doc_->layerAt(static_cast<int>(idx));
    if (!layer)
        throw std::out_of_range("layer idx");

    if (layer->visible() == visible)
        return;

    apply(commands::makeSetLayerVisibleCommand(doc_.get(), layer->id(), layer->visible(), visible));
}

// NOLINT(bugprone-easily-swappable-parameters)
void AppService::setLayerOpacity(std::size_t idx, float alpha)
{
    if (!doc_)
        throw std::runtime_error("setLayerOpacity: document is null");

    auto layer = doc_->layerAt(static_cast<int>(idx));
    if (!layer)
        throw std::out_of_range("layer idx");

    if (layer->opacity() == alpha)
        return;

    apply(commands::makeSetLayerOpacityCommand(doc_.get(), layer->id(), layer->opacity(), alpha));
}

void AppService::setLayerLocked(std::size_t idx, bool locked)
{
    if (!doc_)
        throw std::runtime_error("setLayerLocked: document is null");

    auto layer = doc_->layerAt(static_cast<int>(idx));
    if (!layer)
        throw std::out_of_range("layer idx");

    if (layer->locked() == locked)
        return;

    apply(commands::makeSetLayerLockedCommand(doc_.get(), layer->id(), layer->locked(), locked));
}

void AppService::addLayer(const LayerSpec& spec)
{
    if (!doc_)
        throw std::runtime_error("AddLayer Error : document is null");

    auto img = std::make_shared<ImageBuffer>(doc_->width(), doc_->height());
    img->fill(spec.color);

    auto layer = std::make_shared<Layer>(nextLayerId_++, spec.name, img, spec.visible, spec.locked,
                                         spec.opacity);

    apply(commands::makeAddLayerCommand(doc_.get(), std::move(layer), &activeLayer_));
}

void AppService::removeLayer(std::size_t idx)
{
    if (!doc_)
        throw std::runtime_error("removeLayer: document is null");

    auto layer = doc_->layerAt(static_cast<int>(idx));
    if (!layer)
        throw std::out_of_range("layer idx");

    if (layer->locked())
        throw std::runtime_error("Cannot remove locked layer");

    apply(
        commands::makeRemoveLayerCommand(doc_.get(), layer, static_cast<int>(idx), &activeLayer_));
}

void AppService::reorderLayer(std::size_t from, std::size_t to)
{
    if (!doc_)
        throw std::runtime_error("reorderLayer: document is null");

    const size_t n = doc_->layerCount();
    if (from >= static_cast<std::size_t>(n) || to >= static_cast<std::size_t>(n))
        throw std::out_of_range("reorderLayer: index out of range");

    if (from == to)
        return;

    auto layer = doc_->layerAt(static_cast<int>(from));
    if (!layer)
        throw std::out_of_range("reorderLayer: invalid from");

    apply(commands::makeReorderLayerCommand(doc_.get(), layer->id(), static_cast<int>(from),
                                            static_cast<int>(to), &activeLayer_));
}

void AppService::mergeLayerDown(std::size_t from)
{
    if (!doc_)
        throw std::runtime_error("mergeLayerDown: document is null");

    const size_t n = doc_->layerCount();
    if (from >= static_cast<std::size_t>(n))
        throw std::out_of_range("mergeLayerDown: index out of range");

    if (from == 0)
        throw std::runtime_error("Cannot merge down background");

    auto layer = doc_->layerAt(static_cast<int>(from));
    if (!layer)
        throw std::out_of_range("mergeLayerDown: invalid index");

    apply(commands::makeMergeDownCommand(doc_.get(), layer, static_cast<int>(from), &activeLayer_));
}

void AppService::beginStroke(const ToolParams& params, common::Point pStart)
{
    if (!doc_)
        throw std::runtime_error("beginStroke: document is null");
    if (currentStroke_)
        throw std::logic_error("beginStroke: stroke already in progress");

    const auto n = doc_->layerCount();
    if (n == 0 || activeLayer_ >= n)
        throw std::runtime_error("beginStroke: invalid active layer");

    auto layer = doc_->layerAt(activeLayer_);
    if (!layer)
        throw std::runtime_error("beginStroke: active layer null");
    if (layer->locked())
        throw std::runtime_error("beginStroke: layer is locked");
    if (!layer->image())
        throw std::runtime_error("beginStroke: layer has no image");

    const std::uint64_t layerId = layer->id();

    ApplyFn applyFn = [doc = doc_.get()](std::uint64_t id, const std::vector<PixelChange>& changes,
                                         bool useBefore)
    {
        if (!doc)
            return;
        auto idx = commands::findLayerIndexById(*doc, id);
        if (!idx)
            return;

        auto layer2 = doc->layerAt(*idx);
        if (!layer2 || !layer2->image())
            return;
        auto img = layer2->image();

        for (const auto& ch : changes)
        {
            img->setPixel(ch.x, ch.y, useBefore ? ch.before : ch.after);
        }
    };
    currentStroke_ =
        std::make_unique<commands::StrokeCommand>(doc_.get(), layerId, params, std::move(applyFn));
    currentStroke_->addPoint(pStart);
}

void AppService::moveStroke(common::Point p)
{
    if (!currentStroke_)
        return;
    currentStroke_->addPoint(p);
}
void AppService::endStroke()
{
    if (!currentStroke_)
        return;
    apply(std::move(currentStroke_));
}

uint32_t AppService::pickColorAt(common::Point p) const
{
    if (!doc_)
        throw std::runtime_error("pickColorAt: document is null");
    const auto n = doc_->layerCount();
    if (n <= 0)
        return common::colors::Transparent;
    if (activeLayer_ >= n)
        return common::colors::Transparent;

    auto layer = doc_->layerAt(activeLayer_);
    if (!layer)
        return common::colors::Transparent;

    auto img = layer->image();
    if (!img)
        return common::colors::Transparent;
    if (p.x < 0 || p.y < 0 || p.x >= img->width() || p.y >= img->height())
        return common::colors::Transparent;

    return img->getPixel(p.x, p.y);
}

const Selection& AppService::selection() const
{
    if (!doc_)
        throw std::runtime_error("selection: document is null");
    return doc_->selection();
}

void AppService::setSelectionRect(Selection::Rect r)
{
    if (!doc_)
        throw std::runtime_error("setSelectionRect: document is null");
    doc_->selection().clear();
    doc_->selection().addRect(r, std::make_shared<ImageBuffer>(doc_->width(), doc_->height()));
    documentChanged.emit();
}

void AppService::clearSelectionRect()
{
    if (!doc_)
        throw std::runtime_error("clearSelectionRect: document is null");
    doc_->selection().clear();
    documentChanged.emit();
}

void AppService::undo()
{
    if (!history_.canUndo())
        return;

    history_.undo();
    documentChanged.emit();
}

void AppService::redo()
{
    if (!history_.canRedo())
        return;

    history_.redo();
    documentChanged.emit();
}
bool AppService::canUndo() const noexcept
{
    return history_.canUndo();
}

bool AppService::canRedo() const noexcept
{
    return history_.canRedo();
}
void AppService::apply(app::History::CommandPtr cmd)
{
    if (!cmd)
        return;

    cmd->redo();
    history_.push(std::move(cmd));
    documentChanged.emit();
}

}  // namespace app