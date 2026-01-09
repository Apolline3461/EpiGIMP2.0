//
// Created by apolline on 09/01/2026.
//
#include <gtest/gtest.h>
#include <memory>

#include "core/Compositor.hpp"
#include "core/Document.hpp"
#include "core/Layer.hpp"
#include "core/ImageBuffer.hpp"

class ImageBuffer;

// Create a layer with a chosen color.
static std::shared_ptr<Layer> makeSolidLayer(
    std::uint64_t id,
    const std::string& name,
    int width,
    int height,
    std::uint32_t color)
{
    auto img = std::make_shared<ImageBuffer>(width, height);
    img->fill(color);
    return std::make_shared<Layer>(id, name, img);
}

// helper rgba
static constexpr std::uint32_t rgba(
    std::uint8_t r,
    std::uint8_t g,
    std::uint8_t b,
    std::uint8_t a)
{
    return (static_cast<std::uint32_t>(r) << 24) |
           (static_cast<std::uint32_t>(g) << 16) |
           (static_cast<std::uint32_t>(b) << 8)  |
           static_cast<std::uint32_t>(a);
}

// -----------------------------------------------------------------------------
// Tests Compositor
// -----------------------------------------------------------------------------

TEST(Compositor, EmptyDocumentProducesTransparentBlack) {
    const Document doc(4, 4);
    ImageBuffer out(4, 4);

    constexpr Compositor comp;
    comp.compose(doc, out);

    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            EXPECT_EQ(out.getPixel(x, y), 0u);
        }
    }
}

TEST(Compositor, SingleLayerCopiesPixels) {
    Document doc(2, 2);
    const auto layer = makeSolidLayer(1, "L1", 2, 2, rgba(0x11, 0x22, 0x33, 0xFF));
    doc.addLayer(layer);

    ImageBuffer out(2, 2);
    constexpr Compositor comp;
    comp.compose(doc, out);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x)
            EXPECT_EQ(out.getPixel(x, y), rgba(0x11, 0x22, 0x33, 0xFF));
    }
}

TEST(Compositor, TopVisibleLayerOverridesBelow) {
    Document doc(2, 2);

    const auto bottom = makeSolidLayer(1, "bottom", 2, 2, rgba(0x11, 0x11, 0x11, 0xFF));
    const auto top    = makeSolidLayer(2, "top",    2, 2, rgba(0x22, 0x22, 0x22, 0xFF));

    doc.addLayer(bottom); // index 0
    doc.addLayer(top);    // index 1

    ImageBuffer out(2, 2);
    constexpr Compositor comp;
    comp.compose(doc, out);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x)
            EXPECT_EQ(out.getPixel(x, y), rgba(0x22, 0x22, 0x22, 0xFF));
    }
}

TEST(Compositor, InvisibleTopLayerRevealsBelow) {
    Document doc(2, 2);

    const auto bottom = makeSolidLayer(1, "bottom", 2, 2, rgba(0x11, 0x11, 0x11, 0xFF));
    const auto top    = makeSolidLayer(2, "top",    2, 2, rgba(0x22, 0x22, 0x22, 0xFF));
    top->setVisible(false);

    doc.addLayer(bottom); // index 0
    doc.addLayer(top);    // index 1

    ImageBuffer out(2, 2);
    constexpr Compositor comp;
    comp.compose(doc, out);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x)
            EXPECT_EQ(out.getPixel(x, y), rgba(0x11, 0x11, 0x11, 0xFF));
    }
}

TEST(Compositor, SemiTransparentTopLayerBlendsWithBelow) {
    Document doc(1, 1);

    const auto bottom = makeSolidLayer(1, "bottom", 1, 1, rgba(0x00, 0x00, 0xFF, 0xFF)); // bleu opaque
    const auto top    = makeSolidLayer(2, "top",    1, 1, rgba(0xFF, 0x00, 0x00, 0x80)); // rouge alpha ~0.5

    doc.addLayer(bottom);
    doc.addLayer(top);

    ImageBuffer out(1, 1);
    constexpr Compositor comp;
    comp.compose(doc, out);

    const auto px = out.getPixel(0, 0);

    EXPECT_NE(px, rgba(0xFF, 0x00, 0x00, 0xFF));
    EXPECT_NE(px, rgba(0x00, 0x00, 0xFF, 0xFF));
}
