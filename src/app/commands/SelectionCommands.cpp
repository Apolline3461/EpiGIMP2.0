//
// SelectionCommands.cpp
//

#include "app/commands/SelectionCommands.hpp"

#include <utility>

#include "app/Command.hpp"
#include "core/Document.hpp"

namespace app::commands
{
class SetSelectionMaskCommand final : public Command
{
   public:
    SetSelectionMaskCommand(Document* doc, std::shared_ptr<ImageBuffer> before,
                            std::shared_ptr<ImageBuffer> after)
        : doc_(doc), before_(std::move(before)), after_(std::move(after))
    {
    }

    void redo() override
    {
        if (!doc_)
            return;
        if (after_)
            doc_->selection().setMask(after_);
        else
            doc_->selection().clear();
    }

    void undo() override
    {
        if (!doc_)
            return;
        if (before_)
            doc_->selection().setMask(before_);
        else
            doc_->selection().clear();
    }

   private:
    Document* doc_{nullptr};
    std::shared_ptr<ImageBuffer> before_;
    std::shared_ptr<ImageBuffer> after_;
};

std::unique_ptr<Command> makeSetSelectionMaskCommand(Document* doc,
                                                     std::shared_ptr<ImageBuffer> before,
                                                     std::shared_ptr<ImageBuffer> after)
{
    return std::make_unique<SetSelectionMaskCommand>(doc, std::move(before), std::move(after));
}

}  // namespace app::commands
