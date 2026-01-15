/*
** EPITECH PROJECT, 2025
** EpiGIMP2.0
** File description:
** Command
*/

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace app
{

struct PixelChange
{
    int x;
    int y;
    std::uint32_t before;
    std::uint32_t after;
};

// apply function: layerId, changes, useBefore => true sets to before, false sets to after
using ApplyFn = std::function<void(std::uint64_t, const std::vector<PixelChange>&, bool)>;

class Command
{
   public:
    virtual ~Command() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
};

// Simple concrete command that applies pixel changes using an ApplyFn
class DataCommand : public Command
{
   public:
    DataCommand(std::uint64_t layerId, std::vector<PixelChange> changes, ApplyFn apply)
        : layerId_{layerId}, changes_{std::move(changes)}, apply_{std::move(apply)}
    {
    }

    void undo() override
    {
        apply_(layerId_, changes_, /*useBefore=*/true);
    }
    void redo() override
    {
        apply_(layerId_, changes_, /*useBefore=*/false);
    }

   private:
    std::uint64_t layerId_{};
    std::vector<PixelChange> changes_;
    ApplyFn apply_;
};

}  // namespace app
