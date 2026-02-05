//
// Created by apolline on 03/02/2026.
//
#include "app/commands/StrokeCommand.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <unordered_map>

#include "app/commands/CommandUtils.hpp"
#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

namespace app::commands
{
namespace
{
static void rasterizeLine(common::Point a, common::Point b,
                          const std::function<void(int, int)>& emitPixel)
{
    int x0 = a.x, y0 = a.y;
    int x1 = b.x, y1 = b.y;

    int dx = std::abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
        emitPixel(x0, y0);
        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}
}  // namespace

StrokeCommand::StrokeCommand(Document* doc, std::uint64_t layerId, ToolParams params, ApplyFn apply)
    : doc_(doc), layerId_(layerId), params_(params), apply_(std::move(apply))
{
}

void StrokeCommand::addPoint(common::Point p)
{
    points_.push_back(p);
}

void StrokeCommand::redo()
{
    if (!built_)
        buildChanges();
    apply_(layerId_, changes_, /*useBefore=*/false);
}

void StrokeCommand::undo()
{
    if (!built_)
        return;
    apply_(layerId_, changes_, /*useBefore=*/true);
}

void StrokeCommand::buildChanges()
{
    built_ = true;
    if (!doc_)
        return;

    auto idxOpt = findLayerIndexById(*doc_, layerId_);
    if (!idxOpt)
        return;

    auto layer = doc_->layerAt(*idxOpt);
    if (!layer || !layer->image())
        return;

    auto img = layer->image();
    const int w = img->width();
    const int h = img->height();

    std::unordered_map<std::uint64_t, PixelChange> map;
    map.reserve(256);

    auto record = [&](int x, int y)
    {
        if (x < 0 || y < 0 || x >= w || y >= h)
            return;

        const std::uint64_t key =
            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) << 32) |
            static_cast<std::uint32_t>(x);

        auto it = map.find(key);
        if (it == map.end())
        {
            PixelChange ch{};
            ch.x = x;
            ch.y = y;
            ch.before = img->getPixel(x, y);
            ch.after = params_.color;
            map.emplace(key, ch);
        }
        else
        {
            it->second.after = params_.color;
        }
    };

    if (points_.empty())
        return;

    if (points_.size() == 1)
    {
        record(points_[0].x, points_[0].y);
    }
    else
    {
        for (std::size_t i = 1; i < points_.size(); ++i)
            rasterizeLine(points_[i - 1], points_[i], record);
    }

    changes_.clear();
    changes_.reserve(map.size());

    std::transform(map.begin(), map.end(), std::back_inserter(changes_),
                   [](const auto& kv) { return kv.second; });
}
}  // namespace app::commands
