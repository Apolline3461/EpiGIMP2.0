//
// Created by apolline on 03/02/2026.
//

#pragma once

#include <cstdint>
#include <vector>

#include "app/Command.hpp"
#include "app/ToolParams.hpp"
#include "common/Geometry.hpp"

class Document;

namespace app::commands
{
class StrokeCommand final : public Command
{
   public:
    StrokeCommand(Document* doc, std::uint64_t layerId, ToolParams params, ApplyFn apply);

    void addPoint(common::Point p);

    void redo() override;
    void undo() override;

   private:
    void buildChanges();

    Document* doc_{nullptr};
    std::uint64_t layerId_{0};
    ToolParams params_{};
    ApplyFn apply_;

    std::vector<common::Point> points_;
    std::vector<PixelChange> changes_;
    bool built_{false};
};
}  // namespace app::commands
