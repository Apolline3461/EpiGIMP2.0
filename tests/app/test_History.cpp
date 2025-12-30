#include <gtest/gtest.h>

#include <memory>

#include "app/History.hpp"
#include "core/ImageBuffer.h"
#include "core/Layer.h"

using namespace app;

static void applyToLayer(std::shared_ptr<ImageBuffer> img,
                         const std::vector<History::PixelChange>& ch, bool useBefore)
{
    for (const auto& c : ch)
    {
        const uint32_t v = useBefore ? c.before : c.after;
        img->setPixel(c.x, c.y, v);
    }
}

TEST(HistoryTest, UndoRedoPixels)
{
    // create image and layer
    auto img = std::make_shared<ImageBuffer>(10, 10);
    img->fill(0x00000000);
    Layer layer(1, "L", img);

    History history(128);

    auto apply = [&](uint64_t layerId, const std::vector<History::PixelChange>& ch, bool useBefore)
    {
        (void)layerId;
        applyToLayer(img, ch, useBefore);
    };

    // push 25 small brush strokes
    for (int s = 0; s < 25; ++s)
    {
        History::Action a;
        a.layerId = 1;
        a.description = "stroke" + std::to_string(s);
        // change a single pixel
        History::PixelChange pc{s % 10, (s / 10), 0x00000000u,
                                static_cast<uint32_t>(0xFF000000u | (s & 0xFF))};
        a.changes.push_back(pc);
        // apply forward and push
        apply(a.layerId, a.changes, false);
        history.push(std::move(a));
    }

    // verify some pixels changed
    EXPECT_EQ(img->getPixel(0, 0) & 0xFF, 0u);
    EXPECT_EQ(img->getPixel(1, 0) & 0xFF, 1u);

    // undo all
    for (int i = 0; i < 25; ++i)
    {
        history.undo(apply);
    }

    // all pixels should be restored to 0
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 10; ++x)
            EXPECT_EQ(img->getPixel(x, y), 0u);

    // redo all
    for (int i = 0; i < 25; ++i)
    {
        history.redo(apply);
    }

    // verify pixels again
    EXPECT_EQ(img->getPixel(0, 0) & 0xFF, 0u);
    EXPECT_EQ(img->getPixel(1, 0) & 0xFF, 1u);
}
