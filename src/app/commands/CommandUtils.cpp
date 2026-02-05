//
// Created by apolline on 03/02/2026.
//

#include "app/commands/CommandUtils.hpp"

#include "core/Document.hpp"
#include "core/Layer.hpp"

namespace app::commands
{
std::optional<std::size_t> findLayerIndexById(const Document& doc, std::uint64_t id)
{
    for (std::size_t i = 0; i < doc.layerCount(); ++i)
    {
        auto layer = doc.layerAt(i);
        if (layer && layer->id() == id)
            return i;
    }
    return std::nullopt;
}

void clampActiveLayer(std::size_t* activeLayer, std::size_t layerCount)
{
    if (!activeLayer)
        return;

    if (layerCount == 0)
    {
        *activeLayer = 0;
        return;
    }
    if (*activeLayer >= layerCount)
        *activeLayer = layerCount - 1;
}
}  // namespace app::commands
