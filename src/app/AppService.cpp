//
// Created by apolline on 21/01/2026.
//
#include "app/AppService.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

#include "app/Command.hpp"
#include "app/commands/CommandUtils.hpp"
#include "app/commands/LayerCommands.hpp"
#include "app/commands/PixelCommands.hpp"
#include "app/commands/SelectionCommands.hpp"
#include "app/ToolParams.hpp"
#include "common/Colors.hpp"
#include "core/BucketFill.hpp"
#include "core/Compositor.hpp"
#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

#include <core/Transform.hpp>

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
    if (!doc_)
        throw std::runtime_error("AppService::document(): no document loaded");
    return *doc_;
}

Document& AppService::document()
{
    if (!doc_)
        throw std::runtime_error("AppService::document(): no document loaded");
    return *doc_;
}

bool AppService::hasDocument() const
{
    return static_cast<bool>(doc_);
}

void AppService::closeDocument()
{
    doc_.reset();
    history_.clear();
    dirty_ = false;
    activeLayer_ = 0;
    nextLayerId_ = 1;
    documentChanged.notify();
}

void AppService::newDocument(Size size, float dpi, std::uint32_t bgColor)
{
    if (size.w <= 0 || size.h <= 0)
        throw std::invalid_argument("newDocument: invalid document size");
    doc_ = std::make_unique<Document>(size.w, size.h, dpi);
    history_.clear();
    dirty_ = false;
    activeLayer_ = 0;
    nextLayerId_ = 1;

    auto img = std::make_shared<ImageBuffer>(size.w, size.h);
    img->fill(bgColor);

    auto layer = std::make_shared<Layer>(0, "Background", img, true, false, 1.f);
    doc_->addLayer(std::move(layer));

    documentChanged.notify();
}

static std::size_t pickEditableLayerIndex(const Document& doc)
{
    if (doc.layerCount() == 0)
        return 0;

    for (std::size_t i = doc.layerCount(); i-- > 0;)
    {
        auto l = doc.layerAt(i);
        if (l && l->visible() && !l->locked() && l->image())
            return i;
    }
    return doc.layerCount() - 1;
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
    dirty_ = false;
    activeLayer_ = pickEditableLayerIndex(*doc_);

    nextLayerId_ = computeNextLayerId(*doc_);
    documentChanged.notify();
}

void AppService::replaceBackgroundWithImage(const ImageBuffer& img, std::string name)
{
    if (!doc_)
        throw std::runtime_error("replaceBackgroundWithImage: document is null");

    if (doc_->layerCount() == 0)
        throw std::runtime_error("replaceBackgroundWithImage: no layers");

    auto bg = doc_->layerAt(0);
    if (!bg || !bg->image())
        throw std::runtime_error("replaceBackgroundWithImage: background layer invalid");

    // copie dans un buffer à la taille du doc
    auto out = std::make_shared<ImageBuffer>(doc_->width(), doc_->height());
    out->fill(0u);

    const int copyW = std::min(out->width(), img.width());
    const int copyH = std::min(out->height(), img.height());

    for (int y = 0; y < copyH; ++y)
        for (int x = 0; x < copyW; ++x)
            out->setPixel(x, y, img.getPixel(x, y));

    bg->setImageBuffer(out);
    bg->setName(std::move(name));

    activeLayer_ = 0;  // logique : on travaille sur le BG qui contient l’image
    history_.clear();  // ouvrir une image = nouvel état, pas d’historique
    documentChanged.notify();
}

void AppService::save(const std::string& path)
{
    if (!storage_)
        throw std::runtime_error("Failed Save: storage is null");
    if (!doc_)
        throw std::runtime_error("Failed Save: document is null");
    storage_->save(*doc_, path);
    dirty_ = false;
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

void AppService::setLayerName(std::size_t idx, const std::string& name)
{
    if (!doc_)
        throw std::runtime_error("setLayerName: document is null");
    auto layer = doc_->layerAt(static_cast<int>(idx));
    if (!layer)
        throw std::out_of_range("layer idx");
    if (layer->locked())
        throw std::runtime_error("Cannot rename locked layer");
    if (layer->name() == name)
        return;

    apply(commands::makeSetLayerNameCommand(doc_.get(), layer->id(), layer->name(), name));
}

void AppService::addLayer(const LayerSpec& spec)
{
    if (!doc_)
        throw std::runtime_error("AddLayer Error : document is null");

    const auto w = spec.width.value_or(doc_->width());
    const auto h = spec.height.value_or(doc_->height());
    if (w == 0 || h == 0)
        throw std::invalid_argument("addLayer: invalid layer size");

    auto img = std::make_shared<ImageBuffer>(w, h);
    img->fill(spec.color);

    auto layer = std::make_shared<Layer>(nextLayerId_++, spec.name, img, spec.visible, spec.locked,
                                         spec.opacity);
    layer->setOffset(spec.offsetX, spec.offsetY);

    apply(commands::makeAddLayerCommand(doc_.get(), std::move(layer), &activeLayer_));
}

void AppService::addImageLayer(const ImageBuffer& img, std::string name, bool visible, bool locked,
                               float opacity)
{
    if (!doc_)
        throw std::runtime_error("addImageLayer: document is null");
    auto out = std::make_shared<ImageBuffer>(img.width(), img.height());
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            out->setPixel(x, y, img.getPixel(x, y));

    auto layer = std::make_shared<Layer>(nextLayerId_++,
                                         name.empty() ? std::string("Layer") : std::move(name), out,
                                         visible, locked, opacity);
    layer->setOffset(0, 0);
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

    auto srcLayer = doc_->layerAt(from);

    if (!srcLayer)
        throw std::out_of_range("mergeLayerDown: invalid index");

    if (srcLayer->locked())
        throw std::runtime_error("Cannot merge locked layer");

    auto dstLayer = doc_->layerAt(from - 1);
    if (!dstLayer)
        throw std::out_of_range("mergeLayerDown: invalid target index");

    if (dstLayer->locked())
        throw std::runtime_error("Cannot merge locked layer");

    apply(commands::makeMergeDownCommand(doc_.get(), srcLayer, from, &activeLayer_));
}

void AppService::moveLayer(std::size_t idx, int newOffsetX, int newOffsetY)
{
    if (!doc_)
        throw std::runtime_error("moveLayer: document is null");

    const auto n = doc_->layerCount();
    if (idx >= n)
        throw std::out_of_range("moveLayer: index out of range");

    if (idx == 0)
        return;  // ou throw si tu préfères interdire le move du background

    auto layer = doc_->layerAt(idx);
    if (!layer)
        throw std::runtime_error("moveLayer: layer is null");

    if (layer->locked())
        throw std::runtime_error("moveLayer: layer locked");

    const common::Point before{layer->offsetX(), layer->offsetY()};
    const common::Point after{newOffsetX, newOffsetY};

    if (before.x == after.x && before.y == after.y)
        return;

    apply(commands::makeMoveLayerCommand(doc_.get(), layer->id(), before, after));
}

static std::shared_ptr<ImageBuffer> scaleNearest(const ImageBuffer& src, int newW, int newH)
{
    auto dst = std::make_shared<ImageBuffer>(newW, newH);
    for (int y = 0; y < newH; ++y)
    {
        const int sy = (y * src.height()) / newH;
        for (int x = 0; x < newW; ++x)
        {
            const int sx = (x * src.width()) / newW;
            dst->setPixel(x, y, src.getPixel(sx, sy));
        }
    }
    return dst;
}

void AppService::resizeLayer(std::size_t idx, int newW, int newH)
{
    if (!doc_)
        throw std::runtime_error("resizeLayer: document is null");
    if (idx == 0)
        throw std::runtime_error("resizeLayer: cannot resize background");
    if (newW <= 0 || newH <= 0)
        throw std::invalid_argument("resizeLayer: invalid size");

    auto layer = doc_->layerAt(idx);
    if (!layer || !layer->image())
        throw std::runtime_error("resizeLayer: invalid layer");
    if (layer->locked())
        throw std::runtime_error("resizeLayer: layer locked");

    const auto before = layer->image();
    if (before->width() == newW && before->height() == newH)
        return;

    auto after = scaleNearest(*before, newW, newH);

    apply(commands::makeResizeLayerCommand(doc_.get(), layer->id(), before, after));
}

void AppService::duplicateLayer(std::size_t idx)
{
    if (!doc_)
        throw std::runtime_error("duplicateLayer: document is null");

    const std::size_t n = doc_->layerCount();
    if (idx >= n)
        throw std::out_of_range("duplicateLayer: index out of range");

    auto src = doc_->layerAt(idx);
    if (!src || !src->image())
        throw std::runtime_error("duplicateLayer: invalid layer");

    // deep copy image
    auto srcImg = src->image();
    auto copyImg = std::make_shared<ImageBuffer>(srcImg->width(), srcImg->height());
    for (int y = 0; y < srcImg->height(); ++y)
        for (int x = 0; x < srcImg->width(); ++x)
            copyImg->setPixel(x, y, srcImg->getPixel(x, y));

    // name suffix
    std::string newName = src->name();
    if (newName.find(" (copy)") == std::string::npos)
        newName += " (copy)";
    else
        newName += " 2";

    auto duplicated = std::make_shared<Layer>(nextLayerId_++, newName, copyImg, src->visible(),
                                              src->locked(), src->opacity());
    duplicated->setOffset(src->offsetX(), src->offsetY());

    const std::size_t insertAt = idx + 1;  // juste au-dessus du layer original
    apply(commands::makeDuplicateLayerCommand(doc_.get(), std::move(duplicated), insertAt,
                                              &activeLayer_));
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
    if (n == 0)
        return common::colors::Transparent;
    if (activeLayer_ >= n)
        return common::colors::Transparent;

    auto layer = doc_->layerAt(activeLayer_);
    if (!layer)
        return common::colors::Transparent;

    auto img = layer->image();
    if (!img)
        return common::colors::Transparent;

    const int lx = p.x - layer->offsetX();
    const int ly = p.y - layer->offsetY();
    if (lx < 0 || ly < 0 || lx >= img->width() || ly >= img->height())
        return common::colors::Transparent;
    return img->getPixel(lx, ly);
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
    if (r.w <= 0 || r.h <= 0)
    {
        documentChanged.notify();
        return;
    }
    doc_->selection().addRect(r, std::make_shared<ImageBuffer>(doc_->width(), doc_->height()));
    documentChanged.notify();
}

void AppService::setSelectionLasso(const std::vector<common::Point>& poly)
{
    if (!doc_)
        throw std::runtime_error("setSelectionLasso: document is null");
    if (poly.empty())
    {
        // clear selection
        auto before = doc_->selection().mask();
        apply(commands::makeSetSelectionMaskCommand(doc_.get(), before, nullptr));
        return;
    }

    const int w = doc_->width();
    const int h = doc_->height();
    auto mask = std::make_shared<ImageBuffer>(w, h);
    mask->fill(0u);

    // point-in-polygon ray casting
    auto pointInPoly = [&](double px, double py)
    {
        bool inside = false;
        for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
        {
            double xi = poly[i].x;
            double yi = poly[i].y;
            double xj = poly[j].x;
            double yj = poly[j].y;

            const bool intersect =
                ((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi + 0.0) + xi);
            if (intersect)
                inside = !inside;
        }
        return inside;
    };

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            if (pointInPoly(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5))
                mask->setPixel(x, y, 0x000000FFu);
        }
    }

    auto before = doc_->selection().mask();
    apply(commands::makeSetSelectionMaskCommand(doc_.get(), before, mask));
}

bool AppService::copySelection()
{
    if (!doc_)
        throw std::runtime_error("copySelection: document is null");

    const Selection& sel = doc_->selection();
    if (!sel.hasMask() || !sel.mask())
        return false;

    const auto bboxOpt = sel.boundingRect();
    if (!bboxOpt.has_value())
        return false;
    const auto bbox = *bboxOpt;
    if (bbox.w <= 0 || bbox.h <= 0)
        return false;

    // compose the document region corresponding to bbox
    ImageBuffer full(bbox.w, bbox.h);
    Compositor::composeROI(*doc_, bbox.x, bbox.y, bbox.w, bbox.h, full);

    ImageBuffer out(bbox.w, bbox.h);
    out.fill(common::colors::Transparent);

    auto mask = sel.mask();
    for (int y = 0; y < bbox.h; ++y)
    {
        for (int x = 0; x < bbox.w; ++x)
        {
            const int gx = bbox.x + x;
            const int gy = bbox.y + y;
            if (gx < 0 || gy < 0 || gx >= mask->width() || gy >= mask->height())
                continue;
            const uint32_t mpx = mask->getPixel(gx, gy);
            if (static_cast<uint8_t>(mpx & 0xFFu) != 0)
            {
                out.setPixel(x, y, full.getPixel(x, y));
            }
        }
    }

    addImageLayer(out, "Copie sélection");
    return true;
}

void AppService::deleteSelection()
{
    if (!doc_)
        throw std::runtime_error("deleteSelection: document is null");

    const Selection& sel = doc_->selection();
    if (sel.hasMask() && sel.mask())
    {
        const std::size_t activeIdx = static_cast<std::size_t>(activeLayer_);
        if (activeIdx == 0)
        {
            // do not delete background content
            return;
        }

        auto layer = doc_->layerAt(static_cast<int>(activeIdx));
        if (!layer || !layer->image())
            return;
        if (layer->locked())
            throw std::runtime_error("deleteSelection: layer locked");

        auto img = layer->image();
        const int lw = img->width();
        const int lh = img->height();

        std::vector<PixelChange> changes;
        changes.reserve(lw * lh / 8);

        const int offX = layer->offsetX();
        const int offY = layer->offsetY();
        for (int y = 0; y < lh; ++y)
        {
            for (int x = 0; x < lw; ++x)
            {
                const int dx = x + offX;
                const int dy = y + offY;
                if (dx < 0 || dy < 0 || dx >= sel.mask()->width() || dy >= sel.mask()->height())
                    continue;
                if (sel.mask()->getPixel(dx, dy) == 0u)
                    continue;

                const std::uint32_t before = img->getPixel(x, y);
                const std::uint32_t after = common::colors::Transparent;
                if (before != after)
                    changes.push_back(PixelChange{x, y, before, after});
            }
        }

        if (!changes.empty())
        {
            const std::uint64_t layerId = layer->id();
            apply(commands::makePixelChangesCommand(doc_.get(), layerId, std::move(changes)));
        }

        // clear selection (undoable)
        auto beforeMask = sel.mask();
        apply(commands::makeSetSelectionMaskCommand(doc_.get(), beforeMask, nullptr));
        return;
    }

    // no selection mask -> delete entire layer (uses existing logic),
    // but do not allow deleting the background layer at index 0.
    const std::size_t idx = activeLayer_;
    if (idx == 0)
        return;
    removeLayer(idx);
}

void AppService::clearSelectionRect()
{
    if (!doc_)
        throw std::runtime_error("clearSelectionRect: document is null");
    doc_->selection().clear();
    documentChanged.notify();
}

void AppService::bucketFill(common::Point p, std::uint32_t rgba)
{
    if (!doc_)
        throw std::runtime_error("bucketFill: document is null");

    const auto n = doc_->layerCount();
    if (n == 0 || activeLayer_ >= n)
        return;

    auto layer = doc_->layerAt(activeLayer_);
    if (!layer || !layer->image())
        return;
    if (layer->locked())
        throw std::runtime_error("bucketFill: layer locked");

    auto img = layer->image();
    const int w = img->width();
    const int h = img->height();

    //layer local pos
    const int lx = p.x - layer->offsetX();
    const int ly = p.y - layer->offsetY();

    if (lx < 0 || ly < 0 || lx >= w || ly >= h)
        return;

    /* Selection handling */
    const Selection& sel = doc_->selection();
    if (sel.hasMask())
    {
        if (!sel.mask())
            return;
        if (sel.t_at(p.x, p.y) == 0)
            return;
    }

    // --- Work on a copy to compute tracked changes WITHOUT mutating the document yet

    std::vector<std::tuple<int, int, std::uint32_t>> changedPixels;
    if (sel.hasMask() && sel.mask())
    {
        ImageBuffer maskLocal(w, h);
        maskLocal.fill(0u);

        const int offX = layer->offsetX();
        const int offY = layer->offsetY();

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const int dx = x + offX;
                const int dy = y + offY;

                // outside doc -> not selected
                if (dx < 0 || dy < 0 || dx >= doc_->width() || dy >= doc_->height())
                {
                    maskLocal.setPixel(x, y, 0u);
                    continue;
                }
                const std::uint8_t t = sel.t_at(dx, dy);
                maskLocal.setPixel(x, y, t ? 0xFFFFFFFFu : 0u);
            }
        }
        changedPixels =
            core::floodFillWithinMaskCollect(*img, maskLocal, lx, ly, core::Color{rgba});
    }
    else
    {
        changedPixels = core::floodFillCollect(*img, lx, ly, core::Color{rgba});
    }

    if (changedPixels.empty())
        return;

    // --- Convert to PixelChange list (before/after) for DataCommand
    std::vector<PixelChange> changes;
    changes.reserve(changedPixels.size());
    for (const auto& [x, y, oldColor] : changedPixels)
        changes.push_back(PixelChange{x, y, oldColor, rgba});

    const std::uint64_t layerId = layer->id();
    apply(commands::makePixelChangesCommand(doc_.get(), layerId, std::move(changes)));
}

void AppService::rotateLayer(std::size_t idx, float angle)
{
    if (!doc_)
        throw std::runtime_error("rotateLayer: document is null");
    const auto n = doc_->layerCount();
    if (idx >= n)
        throw std::runtime_error("rotateLayer: index out of range");

    auto layer = doc_->layerAt(idx);
    if (!layer || !layer->image())
        throw std::runtime_error("rotateLayer: layer is null");
    if (layer->locked())
        throw std::runtime_error("rotateLayer: layer locked");

    const auto layerId = layer->id();

    const int oldW = layer->image()->width();
    const int oldH = layer->image()->height();
    const common::Point beforeOff{layer->offsetX(), layer->offsetY()};

    auto before = std::make_shared<ImageBuffer>(*layer->image());

    auto after = core::rotate(*before, angle, common::colors::Transparent);
    if (!after)
        throw std::runtime_error("rotateLayer: rotation failed");

    const int newW = after->width();
    const int newH = after->height();

    // garder le centre au même endroit
    const int dx = (oldW - newW) / 2;
    const int dy = (oldH - newH) / 2;
    const common::Point afterOff{beforeOff.x + dx, beforeOff.y + dy};

    apply(app::commands::makeRotateLayerCommand(doc_.get(), layerId, before, after, beforeOff,
                                                afterOff));
}

void AppService::undo()
{
    if (!history_.canUndo())
        return;

    history_.undo();
    documentChanged.notify();
}

void AppService::redo()
{
    if (!history_.canRedo())
        return;

    history_.redo();
    documentChanged.notify();
}

bool AppService::canUndo() const noexcept
{
    return history_.canUndo();
}

bool AppService::canRedo() const noexcept
{
    return history_.canRedo();
}

void AppService::apply(History::CommandPtr cmd)
{
    if (!cmd)
        return;

    cmd->redo();
    history_.push(std::move(cmd));
    dirty_ = true;
    documentChanged.notify();
}

}  // namespace app