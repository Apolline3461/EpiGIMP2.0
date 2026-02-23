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

TEST(Compositor, RoiSinglePixel) {
    Document doc(4, 4);

    auto img = std::make_shared<ImageBuffer>(4, 4);
    img->fill(rgba(0, 0, 0, 0));

    img->setPixel(1, 1, rgba(0xAA, 0xBB, 0xCC, 0xFF));

    const auto layer = std::make_shared<Layer>(1, "L1", img);
    doc.addLayer(layer);

    ImageBuffer out(1, 1);
    constexpr Compositor comp;
    comp.composeROI(doc, 1, 1, 1, 1, out);

    EXPECT_EQ(out.getPixel(0, 0), rgba(0xAA, 0xBB, 0xCC, 0xFF));
}

TEST(Compositor, RoiHorizontalStrip) {
    Document doc(4, 4);

    auto img = std::make_shared<ImageBuffer>(4, 4);
    img->fill(rgba(0, 0, 0, 0));

    constexpr std::uint32_t c1 = rgba(0x10, 0x20, 0x30, 0xFF);
    constexpr std::uint32_t c2 = rgba(0x40, 0x50, 0x60, 0xFF);
    img->setPixel(1, 2, c1);
    img->setPixel(2, 2, c2);

    const auto layer = std::make_shared<Layer>(1, "L1", img);
    doc.addLayer(layer);

    // ROI : x=1, y=2, w=2, h=1
    ImageBuffer out(2, 1);
    constexpr Compositor comp;
    comp.composeROI(doc, 1, 2, 2, 1, out);

    EXPECT_EQ(out.getPixel(0, 0), c1);
    EXPECT_EQ(out.getPixel(1, 0), c2);
}

TEST(Compositor, OffsetLayer_IsCompositedAtCorrectDocPosition)
{
    Document doc(5, 5, 72.f);

    auto bgImg = std::make_shared<ImageBuffer>(5, 5);
    bgImg->fill(0x000000FFu); // noir opaque
    auto bg = std::make_shared<Layer>(0, "bg", bgImg, true, false, 1.f);
    doc.addLayer(bg);

    auto layerImg = std::make_shared<ImageBuffer>(1, 1);
    layerImg->fill(0xFF0000FFu); // rouge opaque
    auto L1 = std::make_shared<Layer>(1, "L1", layerImg, true, false, 1.f);
    L1->setOffset(2, 3); // <-- important
    doc.addLayer(L1);

    ImageBuffer out(5, 5);
    out.fill(0u);

    Compositor c;
    c.compose(doc, out);

    // le pixel (2,3) doit être rouge
    EXPECT_EQ(out.getPixel(2, 3), 0xFF0000FFu);

    // un pixel voisin doit rester noir (bg)
    EXPECT_EQ(out.getPixel(0, 0), 0x000000FFu);
}

