//
// Created by apolline on 03/02/2026.
//

#include "app/commands/LayerCommands.hpp"

#include <stdexcept>

#include "app/commands/CommandUtils.hpp"
#include "core/Document.hpp"
#include "core/Layer.hpp"

namespace app::commands
{
namespace
{

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

        if (findLayerIndexById(*doc_, layer_->id()).has_value())
            return;

        doc_->addLayer(layer_);
        if (activeLayer_)
            *activeLayer_ = doc_->layerCount() - 1;
    }

    void undo() override
    {
        if (!doc_ || !layer_)
            return;

// GCC 15 false positive: warns about dangling pointer even though
// removeLayer() takes size_t by value. Safe to suppress.
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif
#endif
        auto idx = findLayerIndexById(*doc_, layer_->id());
        if (idx.has_value())
            doc_->removeLayer(idx.value());
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
#endif
        clampActiveLayer(activeLayer_, doc_->layerCount());
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

        auto idx = findLayerIndexById(*doc_, layerId_);
        if (!idx)
            return;

        auto layer = doc_->layerAt(*idx);
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

        auto idx = findLayerIndexById(*doc_, layerId_);
        if (!idx)
            return;

        auto layer = doc_->layerAt(*idx);
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
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
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

        auto idx = findLayerIndexById(*doc_, layerId_);
        if (!idx)
            return;

        auto layer = doc_->layerAt(*idx);
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
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    RemoveLayerCommand(Document* doc, std::shared_ptr<Layer> removed, std::size_t index,
                       std::size_t* activeLayer)
        : doc_(doc), removed_(std::move(removed)), index_(index), activeLayer_(activeLayer)
    {
    }

    void redo() override
    {
        if (!doc_ || !removed_)
            return;

        auto idx = findLayerIndexById(*doc_, removed_->id());
        if (!idx)
            return;

        auto layer = doc_->layerAt(*idx);
        if (layer && layer->locked())
            throw std::runtime_error("Cannot remove locked layer");

        doc_->removeLayer(*idx);
        clampActiveLayer(activeLayer_, doc_->layerCount());
    }

    void undo() override
    {
        if (!doc_ || !removed_)
            return;

        std::size_t n = doc_->layerCount();
        std::size_t insertAt = (index_ > n) ? n : index_;

        doc_->addLayer(removed_, insertAt);
        if (activeLayer_)
            *activeLayer_ = insertAt;
    }

   private:
    Document* doc_{nullptr};
    std::shared_ptr<Layer> removed_;
    std::size_t index_{0};
    std::size_t* activeLayer_{nullptr};
};

class ReorderLayerCommand final : public Command
{
   public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    ReorderLayerCommand(Document* doc, std::uint64_t layerId, std::size_t from, std::size_t to,
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
    void moveTo(std::size_t target)
    {
        if (!doc_)
            return;

        std::size_t n = doc_->layerCount();
        if (n == 0)
            return;

        auto curOpt = findLayerIndexById(*doc_, layerId_);
        if (!curOpt)
            return;

        std::size_t cur = *curOpt;
        std::size_t t = (target >= n) ? (n - 1) : target;

        if (cur == t)
            return;

        doc_->reorderLayer(cur, t);

        if (activeLayer_)
        {
            auto after = findLayerIndexById(*doc_, layerId_);
            if (after)
                *activeLayer_ = *after;
        }
    }

    Document* doc_{nullptr};
    std::uint64_t layerId_{0};
    std::size_t from_{0};
    std::size_t to_{0};
    std::size_t* activeLayer_{nullptr};
};

class MergeDownCommand final : public Command
{
   public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    MergeDownCommand(Document* doc, std::shared_ptr<Layer> removed, std::size_t from,
                     std::size_t* activeLayer)
        : doc_(doc), removed_(std::move(removed)), from_(from), activeLayer_(activeLayer)
    {
    }

    void redo() override
    {
        if (!doc_ || !removed_)
            return;

        auto idx = findLayerIndexById(*doc_, removed_->id());
        if (!idx)
            return;

        if (*idx == 0)
            throw std::runtime_error("Cannot merge down background");

        doc_->mergeDown(*idx);
        clampActiveLayer(activeLayer_, doc_->layerCount());
    }

    void undo() override
    {
        if (!doc_ || !removed_)
            return;

        std::size_t n = doc_->layerCount();
        std::size_t insertAt = (from_ > n) ? n : from_;

        doc_->addLayer(removed_, insertAt);
        if (activeLayer_)
            *activeLayer_ = insertAt;
    }

   private:
    Document* doc_{nullptr};
    std::shared_ptr<Layer> removed_;
    std::size_t from_{0};
    std::size_t* activeLayer_{nullptr};
};

}  // namespace

std::unique_ptr<Command> makeAddLayerCommand(Document* doc, std::shared_ptr<Layer> layer,
                                             std::size_t* activeLayer)
{
    return std::make_unique<AddLayerCommand>(doc, std::move(layer), activeLayer);
}

std::unique_ptr<Command> makeRemoveLayerCommand(Document* doc, std::shared_ptr<Layer> removed,
                                                std::size_t index, std::size_t* activeLayer)
{
    return std::make_unique<RemoveLayerCommand>(doc, std::move(removed), index, activeLayer);
}

std::unique_ptr<Command> makeReorderLayerCommand(Document* doc, std::uint64_t layerId,
                                                 std::size_t from, std::size_t to,
                                                 std::size_t* activeLayer)
{
    return std::make_unique<ReorderLayerCommand>(doc, layerId, from, to, activeLayer);
}

std::unique_ptr<Command> makeMergeDownCommand(Document* doc, std::shared_ptr<Layer> removed,
                                              std::size_t from, std::size_t* activeLayer)
{
    return std::make_unique<MergeDownCommand>(doc, std::move(removed), from, activeLayer);
}

std::unique_ptr<Command> makeSetLayerLockedCommand(Document* doc, std::uint64_t layerId,
                                                   bool before, bool after)
{
    return std::make_unique<SetLayerLockedCommand>(doc, layerId, before, after);
}

std::unique_ptr<Command> makeSetLayerVisibleCommand(Document* doc, std::uint64_t layerId,
                                                    bool before, bool after)
{
    return std::make_unique<SetLayerVisibleCommand>(doc, layerId, before, after);
}

std::unique_ptr<Command> makeSetLayerOpacityCommand(Document* doc, std::uint64_t layerId,
                                                    float before, float after)
{
    return std::make_unique<SetLayerOpacityCommand>(doc, layerId, before, after);
}

}  // namespace app::commands
