//
// Created by apolline on 21/01/2026.
//
#include "app/AppService.hpp"

#include <algorithm>
#include <stdexcept>

#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

namespace app
{
static std::uint64_t computeNextLayerId(const Document& doc)
{
    std::uint64_t maxId = 0;
    for (int i = 0; i < doc.layerCount(); ++i)
    {
        if (auto layer = doc.layerAt(i))
        {
            maxId = std::max(maxId, layer->id());
        }
    }
    return maxId + 1;
}

static int findLayerIndexById(const Document& doc, std::uint64_t id)
{
    for (int i = 0; i < doc.layerCount(); ++i)
        if (auto l = doc.layerAt(i); l && l->id() == id)
            return i;
    return -1;
}

class AddLayerCommand final : public Command
{
   public:
    AddLayerCommand(Document* doc, std::shared_ptr<Layer> layer, std::size_t* activeLayer)
        : doc_(doc), layer_(std::move(layer)), activeLayer_(activeLayer)
    {
    }

    void redo() override
    {
        if (!doc_ || !layer_)
            return;

        if (findLayerIndexById(*doc_, layer_->id()) != -1)
            return;

        doc_->addLayer(layer_);
        if (activeLayer_)
            *activeLayer_ = static_cast<std::size_t>(doc_->layerCount() - 1);
    }

    void undo() override
    {
        if (!doc_ || !layer_)
            return;

        const int idx = findLayerIndexById(*doc_, layer_->id());
        if (idx != -1)
            doc_->removeLayer(idx);

        if (activeLayer_)
        {
            const int n = doc_->layerCount();
            if (n <= 0)
                *activeLayer_ = 0;
            else if (*activeLayer_ >= static_cast<std::size_t>(n))
                *activeLayer_ = static_cast<std::size_t>(n - 1);
        }
    }

   private:
    Document* doc_{nullptr};
    std::shared_ptr<Layer> layer_;
    std::size_t* activeLayer_{nullptr};
};

AppService::AppService(std::unique_ptr<IStorage> storage) : storage_(std::move(storage)) {}

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

const Document& AppService::document() const
{
    return *doc_;
}

std::size_t AppService::activeLayer() const
{
    return activeLayer_;
}

void AppService::setActiveLayer(const std::size_t idx)
{
    if (!doc_)
        throw std::runtime_error("setActiveLayer: document is null");

    const int count = doc_->layerCount();
    if (idx >= static_cast<std::size_t>(count))
    {
        throw std::out_of_range("From setActiveLayer: index out of range");
    }
    activeLayer_ = idx;
}

void AppService::addLayer(const LayerSpec& spec)
{
    if (!doc_)
        throw std::runtime_error("AddLayer Error : document is null");

    auto img = std::make_shared<ImageBuffer>(doc_->width(), doc_->height());
    img->fill(spec.color);

    auto layer = std::make_shared<Layer>(nextLayerId_++, spec.name, img, spec.visible, spec.locked,
                                         spec.opacity);

    apply(std::make_unique<AddLayerCommand>(doc_.get(), std::move(layer), &activeLayer_));
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

    doc_->removeLayer(static_cast<int>(idx));

    if (doc_->layerCount() == 0)
    {
        activeLayer_ = 0;
    }
    else if (activeLayer_ >= static_cast<std::size_t>(doc_->layerCount()))
    {
        activeLayer_ = static_cast<std::size_t>(doc_->layerCount() - 1);
    }
    documentChanged.emit();
}
void AppService::setLayerLocked(std::size_t idx, bool locked)
{
    auto layer = doc_->layerAt(static_cast<int>(idx));
    if (!layer)
        throw std::out_of_range("layer idx");
    layer->setLocked(locked);
    documentChanged.emit();
}

void AppService::apply(app::History::CommandPtr cmd)
{
    if (!cmd)
        return;

    cmd->redo();
    history_.push(std::move(cmd));
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

}  // namespace app