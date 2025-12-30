
/*
** EPITECH PROJECT, 2025
** EpiGIMP2.0
** File description:
** History
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace app
{

class History
{
   public:
    struct PixelChange
    {
        int x;
        int y;
        std::uint32_t before;
        std::uint32_t after;
    };

    struct Action
    {
        std::uint64_t layerId{};
        std::string description;
        std::vector<PixelChange> changes;
    };

    // apply function: layerId, changes, useBefore => true sets to before, false sets to after
    using ApplyFn = std::function<void(std::uint64_t, const std::vector<PixelChange>&, bool)>;

    explicit History(std::size_t maxDepth = 256);

    void push(Action action);
    void undo(const ApplyFn& apply);
    void redo(const ApplyFn& apply);
    void clear();

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

   private:
    std::vector<Action> actions_;
    std::size_t index_{static_cast<std::size_t>(-1)};
    std::size_t maxDepth_{};
};

}  // namespace app
