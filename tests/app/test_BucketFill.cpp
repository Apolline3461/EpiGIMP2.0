//
// Created by apolline on 20/02/2026.
//
#include <gtest/gtest.h>
#include <memory>

#include "app/AppService.hpp"
#include "common/Geometry.hpp"
#include "common/Colors.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"
#include "AppServiceUtilsForTest.hpp"

TEST(AppService_BucketFill, NoSelection_FillsAndUndoRedoWorks)
{
    const auto app = makeApp();
    app->newDocument(app::Size{5, 5}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = common::colors::Transparent;
    app->addLayer(spec);
    app->setActiveLayer(1);

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);

    img->setPixel(2, 2, 0xFF000000u); // zone source
    const uint32_t fill = 0xFFFF0000u;

    app->bucketFill(common::Point{2, 2}, fill);

    EXPECT_TRUE(app->canUndo());
    EXPECT_EQ(img->getPixel(2, 2), fill);

    app->undo();
    EXPECT_EQ(img->getPixel(2, 2), 0xFF000000u);

    app->redo();
    EXPECT_EQ(img->getPixel(2, 2), fill);
}

TEST(AppService_BucketFill, WithSelection_ClickOutside_IsNoOpAndNoHistory)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFF00FF00u; // vert
    app->addLayer(spec);
    app->setActiveLayer(1);

    // sélection au centre (2..3, 2..3)
    app->setSelectionRect(Selection::Rect{2, 2, 2, 2});

    const bool beforeUndo = app->canUndo();
    const bool beforeRedo = app->canRedo();

    // clic hors sélection
    app->bucketFill(common::Point{0, 0}, 0xFFFF0000u);

    // rien ne doit changer côté history
    EXPECT_EQ(app->canUndo(), beforeUndo);
    EXPECT_EQ(app->canRedo(), beforeRedo);
}

TEST(AppService_BucketFill, WithSelection_FillsOnlyInsideMask)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu; // blanc partout
    app->addLayer(spec);
    app->setActiveLayer(1);

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);

    // sélection (1..4,1..4) => 4x4
    app->setSelectionRect(Selection::Rect{1, 1, 4, 4});

    const uint32_t fill = 0xFF112233u;
    app->bucketFill(common::Point{2, 2}, fill);

    // dans la sélection
    EXPECT_EQ(img->getPixel(2, 2), fill);

    // hors sélection (0,0) doit rester blanc
    EXPECT_EQ(img->getPixel(0, 0), 0xFFFFFFFFu);
}

TEST(AppService_BucketFill, LockedLayer_Throws)
{
    const auto app = makeApp();
    app->newDocument(app::Size{4, 4}, 72.f);

    app::LayerSpec spec{};
    spec.locked = true;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    EXPECT_THROW(app->bucketFill(common::Point{1, 1}, 0xFF000000u), std::runtime_error);
}

TEST(AppService_BucketFill, OutOfBounds_IsNoOpAndNoHistory)
{
    const auto app = makeApp();
    app->newDocument(app::Size{4, 4}, 72.f);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu;
    app->addLayer(spec);
    app->setActiveLayer(1);

    const bool beforeUndo = app->canUndo();
    app->bucketFill(common::Point{-1, 0}, 0xFF000000u);
    app->bucketFill(common::Point{0, -1}, 0xFF000000u);
    app->bucketFill(common::Point{4, 0}, 0xFF000000u);
    app->bucketFill(common::Point{0, 4}, 0xFF000000u);

    EXPECT_EQ(app->canUndo(), beforeUndo);
}

TEST(AppService_BucketFill, FillsExactlyConnectedRegion_AndUndoRedoRestoresAll)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f, common::colors::Transparent);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = common::colors::Transparent;
    app->addLayer(spec);
    app->setActiveLayer(1);

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);

    const std::uint32_t OUTSIDE = common::colors::Transparent; // 0u
    const std::uint32_t BORDER  = 0xFF0000FFu;                // red (RGBA packed)
    const std::uint32_t TARGET  = 0xFF00FF00u;                // green
    const std::uint32_t FILL    = 0xFFFF0000u;                // yellow-ish (R=FF,G=FF,B=00,A=00? depends packing)
                                                         // adjust if you want fully opaque: 0xFFFF00FFu

    // Build a "walled" region:
    // Border rectangle from (1,1) to (4,4), interior is (2..3,2..3)
    // Outside stays transparent
    for (int y = 1; y <= 4; ++y)
    {
        for (int x = 1; x <= 4; ++x)
        {
            const bool isBorder = (x == 1 || x == 4 || y == 1 || y == 4);
            img->setPixel(x, y, isBorder ? BORDER : TARGET);
        }
    }

    // Fill inside at (2,2): should only affect the interior TARGET pixels (2..3,2..3)
    app->bucketFill(common::Point{2, 2}, FILL);

    // Verify outside unchanged
    EXPECT_EQ(img->getPixel(0, 0), OUTSIDE);
    EXPECT_EQ(img->getPixel(5, 5), OUTSIDE);

    // Verify border unchanged
    for (int x = 1; x <= 4; ++x)
    {
        EXPECT_EQ(img->getPixel(x, 1), BORDER);
        EXPECT_EQ(img->getPixel(x, 4), BORDER);
    }
    for (int y = 1; y <= 4; ++y)
    {
        EXPECT_EQ(img->getPixel(1, y), BORDER);
        EXPECT_EQ(img->getPixel(4, y), BORDER);
    }

    // Verify interior filled
    for (int y = 2; y <= 3; ++y)
        for (int x = 2; x <= 3; ++x)
            EXPECT_EQ(img->getPixel(x, y), FILL);

    // Undo: interior back to TARGET
    ASSERT_TRUE(app->canUndo());
    app->undo();

    for (int y = 2; y <= 3; ++y)
        for (int x = 2; x <= 3; ++x)
            EXPECT_EQ(img->getPixel(x, y), TARGET);

    // Redo: interior back to FILL
    ASSERT_TRUE(app->canRedo());
    app->redo();

    for (int y = 2; y <= 3; ++y)
        for (int x = 2; x <= 3; ++x)
            EXPECT_EQ(img->getPixel(x, y), FILL);
}

TEST(AppService_BucketFill, RespectsLayerOffset_LocalCoords_AndNoOpOutsideLayer)
{
    const auto app = makeApp();
    app->newDocument(app::Size{10, 10}, 72.f, common::colors::Transparent);

    // 3x3 layer at offset (4,4), filled with SOURCE
    const std::uint32_t SOURCE = 0x11223344u;
    const std::uint32_t FILL   = 0xAABBCCDDu;

    app::LayerSpec spec{};
    spec.name = "tiny";
    spec.width = 3;
    spec.height = 3;
    spec.color = SOURCE;
    spec.locked = false;
    spec.offsetX = 4;
    spec.offsetY = 4;

    app->addLayer(spec);
    app->setActiveLayer(1);

    auto layer = app->document().layerAt(1);
    ASSERT_NE(layer, nullptr);
    auto img = layer->image();
    ASSERT_NE(img, nullptr);
    ASSERT_EQ(img->width(), 3);
    ASSERT_EQ(img->height(), 3);

    // --- Click OUTSIDE the layer (doc 3,4 -> local -1,0): must be a no-op + no history push
    {
        const bool beforeUndo = app->canUndo();
        app->bucketFill(common::Point{3, 4}, FILL);

        EXPECT_EQ(app->canUndo(), beforeUndo);

        // ensure pixels unchanged
        for (int y = 0; y < img->height(); ++y)
            for (int x = 0; x < img->width(); ++x)
                EXPECT_EQ(img->getPixel(x, y), SOURCE);
    }

    // --- Click INSIDE the layer (doc 4,4 -> local 0,0): should fill the whole 3x3 region
    app->bucketFill(common::Point{4, 4}, FILL);

    for (int y = 0; y < img->height(); ++y)
        for (int x = 0; x < img->width(); ++x)
            EXPECT_EQ(img->getPixel(x, y), FILL);

    // Undo: restore SOURCE everywhere
    ASSERT_TRUE(app->canUndo());
    app->undo();
    for (int y = 0; y < img->height(); ++y)
        for (int x = 0; x < img->width(); ++x)
            EXPECT_EQ(img->getPixel(x, y), SOURCE);

    // Redo: restore FILL everywhere
    ASSERT_TRUE(app->canRedo());
    app->redo();
    for (int y = 0; y < img->height(); ++y)
        for (int x = 0; x < img->width(); ++x)
            EXPECT_EQ(img->getPixel(x, y), FILL);
}

TEST(AppService_BucketFill, WithSelection_ButMaskEmpty_IsNoOpAndNoHistory)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f, common::colors::Transparent);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = 0xFFFFFFFFu; // blanc
    app->addLayer(spec);
    app->setActiveLayer(1);

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);

    // Met un pixel "source" distinct pour vérifier qu'on ne touche à rien
    img->setPixel(2, 2, 0xFF000000u);

    // Crée une sélection, puis la vide => mask existe mais tout est à 0
    app->setSelectionRect(Selection::Rect{1, 1, 4, 4});
    app->clearSelectionRect();
    // Ici on veut un "mask exists but empty". Or clearSelectionRect() supprime le mask.
    // Donc on force un mask vide explicitement via Selection::setMask(...).
    // (À faire via Document directement.)
    {
        auto& sel = app->document().selection();
        auto emptyMask = std::make_shared<ImageBuffer>(app->document().width(), app->document().height());
        emptyMask->fill(0u);
        sel.setMask(emptyMask);
    }

    const bool beforeUndo = app->canUndo();
    const bool beforeRedo = app->canRedo();

    // Click "dans l'image" mais hors sélection effective (puisque mask vide)
    app->bucketFill(common::Point{2, 2}, 0xFFFF0000u);

    // Aucun historique ajouté
    EXPECT_EQ(app->canUndo(), beforeUndo);
    EXPECT_EQ(app->canRedo(), beforeRedo);

    // Aucun pixel modifié
    EXPECT_EQ(img->getPixel(2, 2), 0xFF000000u);
    EXPECT_EQ(img->getPixel(0, 0), 0xFFFFFFFFu);
}

TEST(AppService_BucketFill, WithSelection_RespectsConnectivityInsideMask)
{
    const auto app = makeApp();
    app->newDocument(app::Size{6, 6}, 72.f, common::colors::Transparent);

    app::LayerSpec spec{};
    spec.locked = false;
    spec.color = common::colors::Transparent;
    app->addLayer(spec);
    app->setActiveLayer(1);

    auto img = app->document().layerAt(1)->image();
    ASSERT_NE(img, nullptr);

    // Sélection large (tout sauf bord, par exemple)
    app->setSelectionRect(Selection::Rect{1, 1, 4, 4});

    // On construit DEUX "îlots" de même couleur SOURCE dans la sélection,
    // séparés par une barrière d'une autre couleur.
    const std::uint32_t SOURCE  = 0xFF111111u;
    const std::uint32_t BARRIER = 0xFF999999u;
    const std::uint32_t FILL    = 0xFF00FF00u;

    // Îlot A (2x2) en haut-gauche de la sélection: (1,1)-(2,2)
    for (int y = 1; y <= 2; ++y)
        for (int x = 1; x <= 2; ++x)
            img->setPixel(x, y, SOURCE);

    // Barrière verticale au milieu: x=3 sur y=1..4
    for (int y = 1; y <= 4; ++y)
        img->setPixel(3, y, BARRIER);

    // Îlot B (2x2) en haut-droite de la sélection: (4,1)-(5,2) mais 5 est hors doc,
    // donc on met (4,1)-(4,2) + (4,1)-(4,2) suffit. Mieux: fais un îlot (4,1)-(4,2) + (4,3)-(4,4) ?
    // Restons simple: 2 pixels empilés.
    img->setPixel(4, 1, SOURCE);
    img->setPixel(4, 2, SOURCE);

    // Click dans l'îlot A
    app->bucketFill(common::Point{1, 1}, FILL);

    // Îlot A doit être rempli
    EXPECT_EQ(img->getPixel(1, 1), FILL);
    EXPECT_EQ(img->getPixel(2, 1), FILL);
    EXPECT_EQ(img->getPixel(1, 2), FILL);
    EXPECT_EQ(img->getPixel(2, 2), FILL);

    // Barrière inchangée
    for (int y = 1; y <= 4; ++y)
        EXPECT_EQ(img->getPixel(3, y), BARRIER);

    // Îlot B NE DOIT PAS être rempli (pas connecté)
    EXPECT_EQ(img->getPixel(4, 1), SOURCE);
    EXPECT_EQ(img->getPixel(4, 2), SOURCE);

    // Undo/Redo doivent restaurer exactement
    ASSERT_TRUE(app->canUndo());
    app->undo();

    EXPECT_EQ(img->getPixel(1, 1), SOURCE);
    EXPECT_EQ(img->getPixel(2, 1), SOURCE);
    EXPECT_EQ(img->getPixel(1, 2), SOURCE);
    EXPECT_EQ(img->getPixel(2, 2), SOURCE);
    for (int y = 1; y <= 4; ++y)
        EXPECT_EQ(img->getPixel(3, y), BARRIER);
    EXPECT_EQ(img->getPixel(4, 1), SOURCE);
    EXPECT_EQ(img->getPixel(4, 2), SOURCE);

    ASSERT_TRUE(app->canRedo());
    app->redo();

    EXPECT_EQ(img->getPixel(1, 1), FILL);
    EXPECT_EQ(img->getPixel(2, 1), FILL);
    EXPECT_EQ(img->getPixel(1, 2), FILL);
    EXPECT_EQ(img->getPixel(2, 2), FILL);
    for (int y = 1; y <= 4; ++y)
        EXPECT_EQ(img->getPixel(3, y), BARRIER);
    EXPECT_EQ(img->getPixel(4, 1), SOURCE);
    EXPECT_EQ(img->getPixel(4, 2), SOURCE);
}
