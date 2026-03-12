//
// SelectionCommands.hpp
//
#pragma once

#include <memory>

#include "app/Command.hpp"
#include "core/ImageBuffer.hpp"

class Document;

namespace app::commands
{
std::unique_ptr<Command> makeSetSelectionMaskCommand(Document* doc,
                                                     std::shared_ptr<ImageBuffer> before,
                                                     std::shared_ptr<ImageBuffer> after);

}  // namespace app::commands
