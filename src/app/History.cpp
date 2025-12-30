
/*
** EPITECH PROJECT, 2025
** EpiGIMP2.0
** File description:
** History
*/

#include "app/History.hpp"

using app::History;

History::History(std::size_t maxDepth) : maxDepth_{maxDepth} {}

void History::push(Action action)
{
    if (action.changes.empty())
        return;

    // If we're not at the end, drop redo history
    if (index_ + 1 < actions_.size())
        actions_.erase(actions_.begin() + index_ + 1, actions_.end());

    actions_.push_back(std::move(action));
    // keep size within maxDepth_
    if (actions_.size() > maxDepth_)
        actions_.erase(actions_.begin(), actions_.begin() + (actions_.size() - maxDepth_));

    index_ = actions_.size() - 1;
}

bool History::canUndo() const noexcept
{
    return !actions_.empty() && index_ != static_cast<std::size_t>(-1);
}

bool History::canRedo() const noexcept
{
    return index_ + 1 < actions_.size();
}

void History::undo(const ApplyFn& apply)
{
    if (!canUndo())
        return;

    const Action& a = actions_[index_];
    // apply inverse: set pixels to before
    apply(a.layerId, a.changes, /*useBefore=*/true);
    if (index_ == 0)
        index_ = static_cast<std::size_t>(-1);
    else
        --index_;
}

void History::redo(const ApplyFn& apply)
{
    if (!canRedo())
        return;

    ++index_;
    const Action& a = actions_[index_];
    // apply forward: set pixels to after
    apply(a.layerId, a.changes, /*useBefore=*/false);
}

void History::clear()
{
    actions_.clear();
    index_ = static_cast<std::size_t>(-1);
}
