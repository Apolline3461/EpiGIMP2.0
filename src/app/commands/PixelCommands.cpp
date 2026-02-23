//
// Created by apolline on 20/02/2026.
//

#include "app/commands/PixelCommands.hpp"

#include "app/commands/CommandUtils.hpp"
#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

namespace app::commands
{
namespace
{
class PixelChangesCommand final : public Command
{
   public:
    PixelChangesCommand(Document* doc, std::uint64_t layerId, std::vector<PixelChange> changes)
        : doc_(doc), layerId_(layerId), changes_(std::move(changes))
    {
    }

    void redo() override
    {
        apply(/*useBefore=*/false);
    }
    void undo() override
    {
        apply(/*useBefore=*/true);
    }

   private:
    void apply(bool useBefore)
    {
        if (!doc_)
            return;

        const auto idx = findLayerIndexById(*doc_, layerId_);
        if (!idx)
            return;

        auto layer = doc_->layerAt(*idx);
        if (!layer || !layer->image())
            return;

        auto img = layer->image();
        for (const auto& c : changes_)
            img->setPixel(c.x, c.y, useBefore ? c.before : c.after);
    }

    Document* doc_{nullptr};
    std::uint64_t layerId_{0};
    std::vector<PixelChange> changes_;
};
}  // namespace

std::unique_ptr<Command> makePixelChangesCommand(Document* doc, std::uint64_t layerId,
                                                 std::vector<PixelChange> changes)
{
    return std::make_unique<PixelChangesCommand>(doc, layerId, std::move(changes));
}
}  // namespace app::commands
