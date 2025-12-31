
/*
** EPITECH PROJECT, 2025
** EpiGIMP2.0
** File description:
** History
*/

#include "app/History.hpp"

using app::History;

History::History(std::size_t maxDepth) : maxDepth_{maxDepth} {}

void History::push(CommandPtr cmd)
{
    if (!cmd)
        return;

    undo_.push_back(std::move(cmd));
    redo_.clear();

    // keep size within maxDepth_
    if (undo_.size() > maxDepth_)
        undo_.erase(undo_.begin(), undo_.begin() + (undo_.size() - maxDepth_));
}

bool History::canUndo() const noexcept
{
    return !undo_.empty();
}

bool History::canRedo() const noexcept
{
    return !redo_.empty();
}

void History::undo()
{
    if (!canUndo())
        return;

    auto cmd = std::move(undo_.back());
    undo_.pop_back();
    cmd->undo();
    redo_.push_back(std::move(cmd));
}

void History::redo()
{
    if (!canRedo())
        return;

    auto cmd = std::move(redo_.back());
    redo_.pop_back();
    cmd->redo();
    undo_.push_back(std::move(cmd));
}

void History::clear()
{
    undo_.clear();
    redo_.clear();
}
