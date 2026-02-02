//
// Created by apolline on 21/01/2026.
//

#include "app/AppService.hpp"

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

AppService::AppService(std::unique_ptr<IStorage> storage)
    : storage_(std::move(storage)), history_(20), activeLayer_(0)
{
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

void AppService::save(const std::string& /*path*/)
{
    throw std::logic_error("TODO AppService::save");
}

void AppService::exportPng(const std::string& /*path*/)
{
    throw std::logic_error("TODO AppService::exportPng");
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
    const int count = doc_->layerCount();
    if (idx >= static_cast<std::size_t>(count))
    {
        throw std::out_of_range("From setActiveLayer: index out of range");
    }
    activeLayer_ = idx;
}

void AppService::addLayer(const LayerSpec& spec)
{
    auto img = std::make_shared<ImageBuffer>(doc_->width(), doc_->height());
    img->fill(spec.color);

    auto layer = std::make_shared<Layer>(nextLayerId_++, spec.name, img, spec.visible, spec.locked,
                                         spec.opacity);

    doc_->addLayer(layer);
    activeLayer_ = static_cast<std::size_t>(doc_->layerCount() - 1);

    documentChanged.emit();
}

void AppService::removeLayer(std::size_t idx)
{
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

void AppService::undo()
{
    throw std::logic_error("TODO AppService::undo");
}
void AppService::redo()
{
    throw std::logic_error("TODO AppService::redo");
}

bool AppService::canUndo() const noexcept
{
    return false;
}
bool AppService::canRedo() const noexcept
{
    return false;
}

}  // namespace app