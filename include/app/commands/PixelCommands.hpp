//
// Created by apolline on 20/02/2026.
//

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "app/Command.hpp"
#include "app/commands/StrokeCommand.hpp"
class Document;

namespace app::commands
{
std::unique_ptr<Command> makePixelChangesCommand(Document* doc, std::uint64_t layerId,
                                                 std::vector<PixelChange> changes);
}
