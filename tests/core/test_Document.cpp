//
// Created by apolline on 09/12/2025.
//

#include <memory>

#include "core/Document.hpp"
#include "core/Layer.hpp"

#include <core/ImageBuffer.hpp>
#include <gtest/gtest.h>

class ImageBuffer;

static std::shared_ptr<Layer> makeLayer(uint64_t id, const std::string& name) {
    return std::make_shared<Layer>(id, name, std::shared_ptr<ImageBuffer>{});
}

// -----------------------------------------------------------------------------
// Basic Tests
// -----------------------------------------------------------------------------

TEST(Document_State, Selection_InitiallyEmpty)
{
    Document doc(10, 10, 72.f);

    EXPECT_FALSE(doc.selection().hasMask());
    EXPECT_EQ(doc.selection().mask(), nullptr);
}

TEST(Document_State, Selection_ConstGetterAvailable)
{
    Document doc(10, 10, 72.f);
    const Document& cdoc = doc;

    // doit compiler
    const auto& sel = cdoc.selection();
    (void)sel;

    EXPECT_FALSE(sel.hasMask());
}

TEST(Document_State, ConstructorInitialState_DefaultArgs) {
    const Document doc(800, 600);

    EXPECT_EQ(doc.width(), 800);
    EXPECT_EQ(doc.height(), 600);
    EXPECT_FLOAT_EQ(doc.dpi(), 72.f);

    EXPECT_EQ(doc.layerCount(), 0);
   // EXPECT_FALSE(doc.selection().hasMask());
}

TEST(Document_State, ConstructorInitialState_CustomDpi) {
    const Document doc(1024, 768, 96.f);

    EXPECT_EQ(doc.width(), 1024);
    EXPECT_EQ(doc.height(), 768);
    EXPECT_FLOAT_EQ(doc.dpi(), 96.f);

    EXPECT_EQ(doc.layerCount(), 0);
    // EXPECT_FALSE(doc.selection().hasMask());
}

TEST(Document, AddLayerAppendsByDefault) {
    Document doc(100, 100);
    const auto L1 = makeLayer(1, "L1");
    const auto L2 = makeLayer(2, "L2");

    const auto idx1 = doc.addLayer(L1);
    const auto idx2 = doc.addLayer(L2);

    EXPECT_EQ(idx1, 0);
    EXPECT_EQ(idx2, 1);
    EXPECT_EQ(doc.layerCount(), 2);
    EXPECT_EQ(doc.layerAt(0), L1);
    EXPECT_EQ(doc.layerAt(1), L2);
}

TEST(Document, AddLayerInsertAtValidIndex) {
    Document doc(100, 100);

    const auto L1 = makeLayer(1, "L1");
    const auto L2 = makeLayer(2, "L2");
    const auto L3 = makeLayer(3, "L3");

    doc.addLayer(L1);
    doc.addLayer(L2);

    auto idxInserted = doc.addLayer(L3, 1);

    EXPECT_EQ(idxInserted, 1);
    EXPECT_EQ(doc.layerCount(), 3);

    EXPECT_EQ(doc.layerAt(0), L1);
    EXPECT_EQ(doc.layerAt(1), L3);
    EXPECT_EQ(doc.layerAt(2), L2);
}

TEST(Document, AddLayerInsertAtInvalidIndex_outOfRange) {
    Document doc(100, 100);

    const auto L1 = makeLayer(1, "L1");
    const auto L2 = makeLayer(2, "L2");
    const auto L3 = makeLayer(3, "L3");

    doc.addLayer(L1);
    ASSERT_EQ(doc.layerCount(), 1);

    auto idxNeg = doc.addLayer(L2, -1);
    EXPECT_FALSE(idxNeg.has_value());
    EXPECT_EQ(doc.layerCount(), 1);
    EXPECT_EQ(doc.layerAt(0), L1);

    auto idxTooBig = doc.addLayer(L3, 5);
    EXPECT_FALSE(idxTooBig.has_value());
    EXPECT_EQ(doc.layerCount(), 1);
    EXPECT_EQ(doc.layerAt(0), L1);
}

TEST(Document, RemoveLayerSingle) {
    Document doc(100, 100);

    auto L1 = makeLayer(1, "L1");
    doc.addLayer(L1);

    ASSERT_EQ(doc.layerCount(), 1);

    doc.removeLayer(0);

    EXPECT_EQ(doc.layerCount(), 0);
}

TEST(Document, RemoveLayerIndexOutOfRangeIsNoOp) {
    Document doc(100, 100);

    auto L1 = makeLayer(1, "L1");
    auto L2 = makeLayer(2, "L2");
    doc.addLayer(L1);
    doc.addLayer(L2);

    ASSERT_EQ(doc.layerCount(), 2);

    doc.removeLayer(-1);
    EXPECT_EQ(doc.layerCount(), 2);
    EXPECT_EQ(doc.layerAt(0), L1);
    EXPECT_EQ(doc.layerAt(1), L2);

    doc.removeLayer(doc.layerCount());
    EXPECT_EQ(doc.layerCount(), 2);
    EXPECT_EQ(doc.layerAt(0), L1);
    EXPECT_EQ(doc.layerAt(1), L2);
}

static std::shared_ptr<Layer> makeLayerWithSolidImage(
    uint64_t id, const std::string& name, int w, int h, uint32_t fill, float opacity = 1.f)
{
    auto img = std::make_shared<ImageBuffer>(w, h);
    img->fill(fill);
    return std::make_shared<Layer>(id, name, img, /*visible*/ true, /*locked*/ false, opacity);
}

TEST(Document_MergeDown, MergeDownOnBottomLayerIsNoOp) {
    Document doc(100, 100);

    auto L1 = makeLayer(1, "L1");
    auto L2 = makeLayer(2, "L2");

    doc.addLayer(L1); // index 0
    doc.addLayer(L2); // index 1

    ASSERT_EQ(doc.layerCount(), 2);

    // mergeDown(0) n'a pas de sens → no-op
    doc.mergeDown(0);

    EXPECT_EQ(doc.layerCount(), 2);
    EXPECT_EQ(doc.layerAt(0), L1);
    EXPECT_EQ(doc.layerAt(1), L2);
}

TEST(Document_MergeDown, BlendsSrcOverDst_WithAlphaAndOpacity)
{
    Document doc(1, 1, 72.f);

    // dst = bleu opaque
    auto dst = makeLayerWithSolidImage(1, "dst", 1, 1, 0x0000FFFFu, 1.f);
    // src = rouge 50% alpha
    auto src = makeLayerWithSolidImage(2, "src", 1, 1, 0xFF000080u, 1.f);

    ASSERT_TRUE(doc.addLayer(dst).has_value()); // index 0
    ASSERT_TRUE(doc.addLayer(src).has_value()); // index 1

    ASSERT_EQ(doc.layerCount(), 2u);
    ASSERT_NE(dst->image(), nullptr);

    doc.mergeDown(1);

    ASSERT_EQ(doc.layerCount(), 1u);

    const uint32_t got = dst->image()->getPixel(0, 0);

    auto ch = [](uint32_t px, int shift) -> int {
        return static_cast<int>((px >> shift) & 0xFFu);
    };

    // attendu : violet opaque ~ (128,0,128,255)
    const uint32_t expected = 0x800080FFu;

    EXPECT_NEAR(ch(got, 24), ch(expected, 24), 1) << "R";
    EXPECT_NEAR(ch(got, 16), ch(expected, 16), 1) << "G";
    EXPECT_NEAR(ch(got,  8), ch(expected,  8), 1) << "B";
    EXPECT_NEAR(ch(got,  0), ch(expected,  0), 1) << "A";
}

TEST(Document, LayerPointersAreShared) {
    Document doc(100, 100);

    auto L1 = makeLayer(1, "L1");
    auto L2 = makeLayer(2, "L2");
    auto L3 = makeLayer(3, "L3");

    doc.addLayer(L1);
    doc.addLayer(L2);
    doc.addLayer(L3);

    ASSERT_EQ(doc.layerCount(), 3);

    // Les shared_ptr stockés dans Document.hpp doivent être les mêmes instances.
    EXPECT_EQ(doc.layerAt(0), L1);
    EXPECT_EQ(doc.layerAt(1), L2);
    EXPECT_EQ(doc.layerAt(2), L3);
}

TEST(Document, DefaultSelectionHasNoMask) {
    Document doc(100, 100);
    EXPECT_FALSE(doc.selection().hasMask());
}

// -----------------------------------------------------------------------------
// Fixture Tests
// -----------------------------------------------------------------------------

class DocumentWithThreeLayers : public ::testing::Test {
protected:
    DocumentWithThreeLayers()
        : doc(100, 100) {}

    void SetUp() override {
        L1 = makeLayer(1, "L1");
        L2 = makeLayer(2, "L2");
        L3 = makeLayer(3, "L3");
        doc.addLayer(L1);
        doc.addLayer(L2);
        doc.addLayer(L3);
    }

    Document doc;
    std::shared_ptr<Layer> L1, L2, L3;
};

TEST_F(DocumentWithThreeLayers, RemoveLayerInMiddle) {
    ASSERT_EQ(doc.layerCount(), 3);
    doc.removeLayer(1);
    EXPECT_EQ(doc.layerCount(), 2);
    EXPECT_EQ(doc.layerAt(0), L1);
    EXPECT_EQ(doc.layerAt(1), L3);
}

TEST_F(DocumentWithThreeLayers, ReorderLayer_MoveIndex2To0) {
    ASSERT_EQ(doc.layerCount(), 3);
    doc.reorderLayer(2, 0); // [L3, L1, L2]

    EXPECT_EQ(doc.layerAt(0), L3);
    EXPECT_EQ(doc.layerAt(1), L1);
    EXPECT_EQ(doc.layerAt(2), L2);
}

TEST_F(DocumentWithThreeLayers, ReorderLayer_MoveIndex0To2) {
    ASSERT_EQ(doc.layerCount(), 3);
    doc.reorderLayer(0, 2); // [L2, L3, L1]

    EXPECT_EQ(doc.layerAt(0), L2);
    EXPECT_EQ(doc.layerAt(1), L3);
    EXPECT_EQ(doc.layerAt(2), L1);
}

TEST_F(DocumentWithThreeLayers, ReorderLayer_MoveToSameIndexIsNoOp) {
    ASSERT_EQ(doc.layerCount(), 3);
    doc.reorderLayer(1, 1); // no-op

    EXPECT_EQ(doc.layerAt(0), L1);
    EXPECT_EQ(doc.layerAt(1), L2);
    EXPECT_EQ(doc.layerAt(2), L3);
}

TEST_F(DocumentWithThreeLayers, ReorderLayerIndexOutOfRangeIsNoOp) {
    ASSERT_EQ(doc.layerCount(), 3);

    // from hors bornes
    doc.reorderLayer(45, 1);
    doc.reorderLayer(3, 1);

    // to hors bornes
    doc.reorderLayer(1, -1);
    doc.reorderLayer(1, 42);

    EXPECT_EQ(doc.layerAt(0), L1);
    EXPECT_EQ(doc.layerAt(1), L2);
    EXPECT_EQ(doc.layerAt(2), L3);
}

TEST_F(DocumentWithThreeLayers, MergeDownReducesLayerCountAndKeepsOrderAbove) {
    ASSERT_EQ(doc.layerCount(), 3);

    // Convention : mergeDown(from) => fusion de layer[from] dans layer[from - 1]
    // Ici on merge L2 dans L1.
    doc.mergeDown(1);

    // On ne teste pas encore l'identité précise du layer résultant à l'index 0,
    // seulement la taille et l'ordre relatif avec L3.
    EXPECT_EQ(doc.layerCount(), 2);
    EXPECT_EQ(doc.layerAt(1), L3); // L3 reste au-dessus
}