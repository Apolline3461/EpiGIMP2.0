/*
** EPITECH PROJECT, 2025
** EpiGIMP2.0
** File description:
** History
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "app/Command.hpp"

namespace app
{

class History
{
   public:
    using CommandPtr = std::unique_ptr<Command>;

    explicit History(std::size_t maxDepth = 20);

    void push(CommandPtr cmd);

    void undo();
    void redo();
    void clear();

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

   private:
    std::vector<CommandPtr> undo_;
    std::vector<CommandPtr> redo_;
    std::size_t maxDepth_{};
};

}  // namespace app
