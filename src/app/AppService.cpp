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

class SetLayerLockedCommand final : public Command
{
   public:
    SetLayerLockedCommand(Document* doc, std::uint64_t layerId, bool before, bool after)
        : doc_(doc), layerId_(layerId), before_(before), after_(after)
    {
    }

    void redo() override
    {
        set(after_);
    }
    void undo() override
    {
        set(before_);
    }

   private:
    void set(bool v)
    {
        if (!doc_)
            return;
        const int idx = findLayerIndexById(*doc_, layerId_);
        if (idx == -1)
            return;
        auto layer = doc_->layerAt(idx);
        if (!layer)
            return;
        layer->setLocked(v);
    }

    Document* doc_{nullptr};
    std::uint64_t layerId_{0};
    bool before_{};
    bool after_{};
};

class SetLayerVisibleCommand final : public Command
{
   public:
    SetLayerVisibleCommand(Document* doc, std::uint64_t layerId, bool before, bool after)
        : doc_(doc), layerId_(layerId), before_(before), after_(after)
    {
    }

    void redo() override
    {
        set(after_);
    }
    void undo() override
    {
        set(before_);
    }

   private:
    void set(bool v)
    {
        if (!doc_)
            return;
        const int idx = findLayerIndexById(*doc_, layerId_);
        if (idx == -1)
            return;
        auto layer = doc_->layerAt(idx);
        if (!layer)
            return;
        layer->setVisible(v);
    }

    Document* doc_{nullptr};
    std::uint64_t layerId_{0};
    bool before_{};
    bool after_{};
};

class SetLayerOpacityCommand final : public Command
{
   public:
    SetLayerOpacityCommand(Document* doc, std::uint64_t layerId, float before, float after)
        : doc_(doc), layerId_(layerId), before_(before), after_(after)
    {
    }

    void redo() override
    {
        set(after_);
    }
    void undo() override
    {
        set(before_);
    }

   private:
    void set(float v)
    {
        if (!doc_)
            return;
        const int idx = findLayerIndexById(*doc_, layerId_);
        if (idx == -1)
            return;
        auto layer = doc_->layerAt(idx);
        if (!layer)
            return;
        layer->setOpacity(v);
    }

    Document* doc_{nullptr};
    std::uint64_t layerId_{0};
    float before_{1.f};
    float after_{1.f};
};

class RemoveLayerCommand final : public Command
{
   public:
    RemoveLayerCommand(Document* doc, std::shared_ptr<Layer> removed, int index,
                       std::size_t* activeLayer)
        : doc_(doc), removed_(std::move(removed)), index_(index), activeLayer_(activeLayer)
    {
    }

    void redo() override
    {
        if (!doc_ || !removed_)
            return;

        const int idx = findLayerIndexById(*doc_, removed_->id());
        if (idx == -1)
            return;

        auto layer = doc_->layerAt(idx);
        if (layer && layer->locked())
            throw std::runtime_error("Cannot remove locked layer");

        doc_->removeLayer(idx);
        clampActive();
    }

    void undo() override
    {
        if (!doc_ || !removed_)
            return;

        const int n = doc_->layerCount();
        int insertAt = index_;
        if (insertAt < 0)
            insertAt = 0;
        if (insertAt > n)
            insertAt = n;

        doc_->addLayer(removed_, insertAt);
        if (activeLayer_)
            *activeLayer_ = static_cast<std::size_t>(insertAt);
    }

   private:
    void clampActive()
    {
        if (!activeLayer_)
            return;
        const int n = doc_->layerCount();
        if (n <= 0)
        {
            *activeLayer_ = 0;
            return;
        }
        if (*activeLayer_ >= static_cast<std::size_t>(n))
            *activeLayer_ = static_cast<std::size_t>(n - 1);
    }

    Document* doc_{nullptr};
    std::shared_ptr<Layer> removed_;
    int index_{-1};
    std::size_t* activeLayer_{nullptr};
};

class ReorderLayerCommand final : public Command
{
   public:
    ReorderLayerCommand(Document* doc, std::uint64_t layerId, int from, int to,
                        std::size_t* activeLayer)
        : doc_(doc), layerId_(layerId), from_(from), to_(to), activeLayer_(activeLayer)
    {
    }

    void redo() override
    {
        moveTo(to_);
    }
    void undo() override
    {
        moveTo(from_);
    }

   private:
    void moveTo(int target)
    {
        if (!doc_)
            return;
        const int n = doc_->layerCount();
        if (n <= 0)
            return;

        const int cur = findLayerIndexById(*doc_, layerId_);
        if (cur == -1)
            return;

        int t = target;
        if (t < 0)
            t = 0;
        if (t >= n)
            t = n - 1;

        if (cur == t)
            return;

        doc_->reorderLayer(cur, t);

        if (activeLayer_)
        {
            const int after = findLayerIndexById(*doc_, layerId_);
            if (after != -1)
                *activeLayer_ = static_cast<std::size_t>(after);
        }
    }

    Document* doc_{nullptr};
    std::uint64_t layerId_{0};
    int from_{0};
    int to_{0};
    std::size_t* activeLayer_{nullptr};
};

class MergeDownCommand final : public Command
{
   public:
    MergeDownCommand(Document* doc, std::shared_ptr<Layer> removed, int from,
                     std::size_t* activeLayer)
        : doc_(doc), removed_(std::move(removed)), from_(from), activeLayer_(activeLayer)
    {
    }

    void redo() override
    {
        if (!doc_ || !removed_)
            return;

        const int idx = findLayerIndexById(*doc_, removed_->id());
        if (idx == -1)
            return;

        if (idx <= 0)
            throw std::runtime_error("Cannot merge down background");

        doc_->mergeDown(idx);
        clampActive();
    }

    void undo() override
    {
        if (!doc_ || !removed_)
            return;

        const int n = doc_->layerCount();
        int insertAt = from_;
        if (insertAt < 0)
            insertAt = 0;
        if (insertAt > n)
            insertAt = n;

        doc_->addLayer(removed_, insertAt);
        if (activeLayer_)
            *activeLayer_ = static_cast<std::size_t>(insertAt);
    }

   private:
    void clampActive()
    {
        if (!activeLayer_)
            return;
        const int n = doc_->layerCount();
        if (n <= 0)
        {
            *activeLayer_ = 0;
            return;
        }
        if (*activeLayer_ >= static_cast<std::size_t>(n))
            *activeLayer_ = static_cast<std::size_t>(n - 1);
    }

    Document* doc_{nullptr};
    std::shared_ptr<Layer> removed_;
    int from_{0};
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

void AppService::setLayerVisible(std::size_t idx, bool visible)
{
    if (!doc_)
        throw std::runtime_error("setLayerVisible: document is null");

    auto layer = doc_->layerAt(static_cast<int>(idx));
    if (!layer)
        throw std::out_of_range("layer idx");

    if (layer->visible() == visible)
        return;

    apply(std::make_unique<SetLayerVisibleCommand>(doc_.get(), layer->id(), layer->visible(),
                                                   visible));
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

    apply(
        std::make_unique<SetLayerOpacityCommand>(doc_.get(), layer->id(), layer->opacity(), alpha));
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

void AppService::setLayerLocked(std::size_t idx, bool locked)
{
    if (!doc_)
        throw std::runtime_error("setLayerLocked: document is null");

    auto layer = doc_->layerAt(static_cast<int>(idx));
    if (!layer)
        throw std::out_of_range("layer idx");

    if (layer->locked() == locked)
        return;

    apply(
        std::make_unique<SetLayerLockedCommand>(doc_.get(), layer->id(), layer->locked(), locked));
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

    apply(std::make_unique<RemoveLayerCommand>(doc_.get(), layer, static_cast<int>(idx),
                                               &activeLayer_));
}
void AppService::reorderLayer(std::size_t from, std::size_t to)
{
    if (!doc_)
        throw std::runtime_error("reorderLayer: document is null");

    const int n = doc_->layerCount();
    if (from >= static_cast<std::size_t>(n) || to >= static_cast<std::size_t>(n))
        throw std::out_of_range("reorderLayer: index out of range");

    if (from == to)
        return;

    auto layer = doc_->layerAt(static_cast<int>(from));
    if (!layer)
        throw std::out_of_range("reorderLayer: invalid from");

    apply(std::make_unique<ReorderLayerCommand>(doc_.get(), layer->id(), static_cast<int>(from),
                                                static_cast<int>(to), &activeLayer_));
}

void AppService::mergeLayerDown(std::size_t from)
{
    if (!doc_)
        throw std::runtime_error("mergeLayerDown: document is null");

    const int n = doc_->layerCount();
    if (from >= static_cast<std::size_t>(n))
        throw std::out_of_range("mergeLayerDown: index out of range");

    if (from == 0)
        throw std::runtime_error("Cannot merge down background");

    auto layer = doc_->layerAt(static_cast<int>(from));
    if (!layer)
        throw std::out_of_range("mergeLayerDown: invalid index");

    apply(std::make_unique<MergeDownCommand>(doc_.get(), layer, static_cast<int>(from),
                                             &activeLayer_));
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