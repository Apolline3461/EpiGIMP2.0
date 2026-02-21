//
// Created by apolline on 03/02/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "app/Command.hpp"

#include <common/Geometry.hpp>

class Document;
class Layer;

namespace app::commands
{
std::unique_ptr<Command> makeAddLayerCommand(Document* doc, std::shared_ptr<Layer> layer,
                                             std::size_t* activeLayer);

std::unique_ptr<Command> makeRemoveLayerCommand(Document* doc, std::shared_ptr<Layer> removed,
                                                std::size_t index, std::size_t* activeLayer);

std::unique_ptr<Command> makeReorderLayerCommand(Document* doc, std::uint64_t layerId,
                                                 std::size_t from, std::size_t to,
                                                 std::size_t* activeLayer);

std::unique_ptr<Command> makeMergeDownCommand(Document* doc, std::shared_ptr<Layer> removed,
                                              std::size_t from, std::size_t* activeLayer);

std::unique_ptr<Command> makeSetLayerLockedCommand(Document* doc, std::uint64_t layerId,
                                                   bool before, bool after);

std::unique_ptr<Command> makeSetLayerVisibleCommand(Document* doc, std::uint64_t layerId,
                                                    bool before, bool after);

std::unique_ptr<Command> makeSetLayerOpacityCommand(Document* doc, std::uint64_t layerId,
                                                    float before, float after);

std::unique_ptr<Command> makeSetLayerNameCommand(Document* doc, std::uint64_t layerId,
                                                 std::string before, std::string after);

std::unique_ptr<Command> makeMoveLayerCommand(Document* doc, std::uint64_t layerId,
                                              common::Point before, common::Point after);

}  // namespace app::commands
