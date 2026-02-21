#include <memory>

#include "app/Command.hpp"
#include "app/History.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

#include <gtest/gtest.h>

using namespace app;

namespace
{
struct CounterCommand final : public Command
{
    int* undoCount{};
    int* redoCount{};
    explicit CounterCommand(int* u, int* r) : undoCount(u), redoCount(r) {}
    void undo() override
    {
        if (undoCount)
            (*undoCount)++;
    }
    void redo() override
    {
        if (redoCount)
            (*redoCount)++;
    }
};
}  // namespace

static void applyToLayer(std::shared_ptr<ImageBuffer> img, const std::vector<app::PixelChange>& ch,
                         bool useBefore)
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

    auto apply = [&](uint64_t layerId, const std::vector<app::PixelChange>& ch, bool useBefore)
    {
        (void)layerId;
        applyToLayer(img, ch, useBefore);
    };

    // push 25 small pencil strokes
    for (int s = 0; s < 25; ++s)
    {
        std::vector<app::PixelChange> changes;
        app::PixelChange pc{s % 10, (s / 10), 0x00000000u,
                            static_cast<uint32_t>(0xFF000000u | (s & 0xFF))};
        changes.push_back(pc);
        // create command, execute forward and push
        auto cmd = std::make_unique<app::DataCommand>(1, std::move(changes), apply);
        cmd->redo();
        history.push(std::move(cmd));
    }

    // verify some pixels changed
    EXPECT_EQ(img->getPixel(0, 0) & 0xFF, 0u);
    EXPECT_EQ(img->getPixel(1, 0) & 0xFF, 1u);

    // undo all
    for (int i = 0; i < 25; ++i)
    {
        history.undo();
    }

    // all pixels should be restored to 0
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 10; ++x)
            EXPECT_EQ(img->getPixel(x, y), 0u);

    // redo all
    for (int i = 0; i < 25; ++i)
    {
        history.redo();
    }

    // verify pixels again
    EXPECT_EQ(img->getPixel(0, 0) & 0xFF, 0u);
    EXPECT_EQ(img->getPixel(1, 0) & 0xFF, 1u);
}

TEST(HistoryTest, PushNullptr_NoEffect)
{
    History h(10);
    EXPECT_FALSE(h.canUndo());
    EXPECT_FALSE(h.canRedo());

    h.push(nullptr);

    EXPECT_FALSE(h.canUndo());
    EXPECT_FALSE(h.canRedo());
}

TEST(HistoryTest, UndoRedo_WhenEmpty_NoOp)
{
    History h(10);

    EXPECT_FALSE(h.canUndo());
    EXPECT_FALSE(h.canRedo());

    // should not crash / should not change state
    h.undo();
    h.redo();
    h.undo();
    h.redo();

    EXPECT_FALSE(h.canUndo());
    EXPECT_FALSE(h.canRedo());
}

TEST(HistoryTest, Push_ClearsRedoStack)
{
    History h(10);

    int uA = 0, rA = 0;
    auto a = std::make_unique<CounterCommand>(&uA, &rA);
    a->redo();
    h.push(std::move(a));

    EXPECT_TRUE(h.canUndo());
    EXPECT_FALSE(h.canRedo());

    h.undo();
    EXPECT_FALSE(h.canUndo());
    EXPECT_TRUE(h.canRedo());

    int uB = 0, rB = 0;
    auto b = std::make_unique<CounterCommand>(&uB, &rB);
    b->redo();
    h.push(std::move(b));

    // redo must be cleared by push()
    EXPECT_TRUE(h.canUndo());
    EXPECT_FALSE(h.canRedo());
}

TEST(HistoryTest, Clear_EmptiesUndoAndRedo)
{
    History h(10);

    int u = 0, r = 0;
    auto cmd = std::make_unique<CounterCommand>(&u, &r);
    cmd->redo();
    h.push(std::move(cmd));

    EXPECT_TRUE(h.canUndo());
    h.undo();
    EXPECT_TRUE(h.canRedo());

    h.clear();
    EXPECT_FALSE(h.canUndo());
    EXPECT_FALSE(h.canRedo());

    // no crash
    h.undo();
    h.redo();
    EXPECT_FALSE(h.canUndo());
    EXPECT_FALSE(h.canRedo());
}

TEST(HistoryTest, MaxDepth_DropsOldestCommands)
{
    History h(3);

    int u = 0, r = 0;

    // push 5 commands
    for (int i = 0; i < 5; ++i)
    {
        auto cmd = std::make_unique<CounterCommand>(&u, &r);
        cmd->redo();
        h.push(std::move(cmd));
    }

    // with depth=3, only 3 undos should be possible
    EXPECT_TRUE(h.canUndo());
    h.undo();
    h.undo();
    h.undo();
    EXPECT_FALSE(h.canUndo());  // the oldest 2 were dropped

    // and redo should now be possible 3 times
    EXPECT_TRUE(h.canRedo());
    h.redo();
    h.redo();
    h.redo();
    EXPECT_FALSE(h.canRedo());

    // counts: 5 redo calls done before push, +3 redo from redo() calls
    EXPECT_EQ(r, 5 + 3);
    EXPECT_EQ(u, 3);
}
