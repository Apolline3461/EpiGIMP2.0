//
// Created by apolline on 03/02/2026.
//
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

class Document;

namespace app::commands
{
std::optional<std::size_t> findLayerIndexById(const Document& doc, std::uint64_t id);
void clampActiveLayer(std::size_t* activeLayer, std::size_t layerCount);
}  // namespace app::commands
