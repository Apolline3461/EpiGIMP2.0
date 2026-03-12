#include <QAction>
#include <QTest>
#include <QToolButton>

#include "ui/CanvasWidget.hpp"
#include "ui/window.hpp"
#include "ui_test_helpers.hpp"

#include <core/ImageBuffer.hpp>
#include <core/Layer.hpp>
#include <gtest/gtest.h>

using namespace ui_test;

namespace
{
QToolButton* btn(QWidget& w, const char* name)
{
    auto* b = w.findChild<QToolButton*>(name);
    EXPECT_NE(b, nullptr) << "Missing button objectName=" << name;
    return b;
}
}  // namespace

TEST(MainWindow_CloseDocument, ResetsToolModesAndActions)
{
    ensureQtApp();

    auto svc = makeService();
    MainWindow w(*svc);

    svc->newDocument({64, 64}, 72.f);
    pumpEvents();

    auto* actBucket = w.findChild<QAction*>("act.bucket");
    auto* actSelect = w.findChild<QAction*>("act.select");
    auto* actColorPick = w.findChild<QAction*>("act.colorPick");
    auto* actClose = w.findChild<QAction*>("act.close");
    auto* actPicker = w.findChild<QAction*>("act.picker");

    ASSERT_NE(actBucket, nullptr);
    ASSERT_NE(actSelect, nullptr);
    ASSERT_NE(actColorPick, nullptr);
    ASSERT_NE(actPicker, nullptr);
    ASSERT_NE(actClose, nullptr);

    actBucket->setChecked(true);
    actSelect->setChecked(true);
    actPicker->setChecked(true);
    pumpEvents();

    actClose->trigger();
    pumpEvents();

    EXPECT_FALSE(actBucket->isChecked());
    EXPECT_FALSE(actSelect->isChecked());
    EXPECT_FALSE(actPicker->isChecked());
    EXPECT_FALSE(actColorPick->isEnabled());
}

TEST(LayersPanelButtons, NoDocument_AllDisabled)
{
    ensureQtApp();

    auto svc = makeService();
    MainWindow w(*svc);

    svc->closeDocument();
    pumpEvents();

    auto* add = btn(w, "btn.layer.add");
    auto* del = btn(w, "btn.layer.del");
    auto* up = btn(w, "btn.layer.up");
    auto* down = btn(w, "btn.layer.down");
    auto* merge = btn(w, "btn.layer.mergeDown");

    EXPECT_FALSE(add->isEnabled());
    EXPECT_FALSE(del->isEnabled());
    EXPECT_FALSE(up->isEnabled());
    EXPECT_FALSE(down->isEnabled());
    EXPECT_FALSE(merge->isEnabled());
}

TEST(LayersPanelButtons, BackgroundSelected_DeleteAndMergeDisabled)
{
    ensureQtApp();

    auto svc = makeService();
    MainWindow w(*svc);

    svc->newDocument({64, 64}, 72.f);
    pumpEvents();

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    ASSERT_GT(list->count(), 0);

    // background = dernière ligne (list top->bottom)
    list->setCurrentRow(list->count() - 1);
    pumpEvents();

    auto* add = btn(w, "btn.layer.add");
    auto* del = btn(w, "btn.layer.del");
    auto* up = btn(w, "btn.layer.up");
    auto* down = btn(w, "btn.layer.down");
    auto* merge = btn(w, "btn.layer.mergeDown");

    EXPECT_TRUE(add->isEnabled());
    EXPECT_FALSE(del->isEnabled());
    EXPECT_FALSE(merge->isEnabled());

    EXPECT_FALSE(up->isEnabled());
    EXPECT_FALSE(down->isEnabled());
}

TEST(LayersPanelButtons, LockedLayerSelected_DeleteAndMergeDisabled)
{
    ensureQtApp();

    auto svc = makeService();
    MainWindow w(*svc);

    svc->newDocument({64, 64}, 72.f);

    app::LayerSpec spec{};
    spec.name = "L1";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0u;
    svc->addLayer(spec);

    svc->setLayerLocked(1, true);
    pumpEvents();

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->count(), 2);

    // row 0 = layer idx 1
    list->setCurrentRow(0);
    pumpEvents();

    auto* del = btn(w, "btn.layer.del");
    auto* merge = btn(w, "btn.layer.mergeDown");

    EXPECT_FALSE(del->isEnabled());
    EXPECT_FALSE(merge->isEnabled());
}

// -----------------------------------------------------------------------------
// Tests Canvas -> MainWindow connects (coverage window.cpp)
// -----------------------------------------------------------------------------

TEST(UI_Canvas, SelectionDrag_UpdatesAppSelectionMask)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);
    pumpEvents();

    auto* actSelect = w.findChild<QAction*>("act.select");
    ASSERT_NE(actSelect, nullptr);
    actSelect->setChecked(true);
    pumpEvents();

    auto* canvas = w.findChild<CanvasWidget*>("canvas");
    ASSERT_NE(canvas, nullptr);

    QTest::mousePress(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QTest::mouseMove(canvas, QPoint(30, 30));
    QTest::mouseRelease(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(30, 30));
    pumpEvents();

    EXPECT_TRUE(svc->selection().hasMask());
    EXPECT_TRUE(svc->selection().boundingRect().has_value());
}

TEST(UI_Canvas, PickerClick_DisablesPickerAction)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    svc->newDocument({16, 16}, 72.f, common::colors::Transparent);
    pumpEvents();

    auto layer = svc->document().layerAt(svc->activeLayer());
    ASSERT_NE(layer, nullptr);
    ASSERT_NE(layer->image(), nullptr);
    layer->image()->setPixel(0, 0, 0xFF112233u);

    auto* actPicker = w.findChild<QAction*>("act.picker");
    ASSERT_NE(actPicker, nullptr);
    actPicker->setChecked(true);
    pumpEvents();

    auto* canvas = w.findChild<CanvasWidget*>("canvas");
    ASSERT_NE(canvas, nullptr);

    QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(0, 0));
    pumpEvents();

    EXPECT_FALSE(actPicker->isChecked());
}

TEST(UI_Canvas, BucketClick_FillsPixel)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    svc->newDocument({16, 16}, 72.f, common::colors::Transparent);
    pumpEvents();

    auto* actBucket = w.findChild<QAction*>("act.bucket");
    ASSERT_NE(actBucket, nullptr);
    actBucket->setChecked(true);
    pumpEvents();

    auto* canvas = w.findChild<CanvasWidget*>("canvas");
    ASSERT_NE(canvas, nullptr);

    QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    pumpEvents();

    auto layer = svc->document().layerAt(svc->activeLayer());
    ASSERT_NE(layer, nullptr);
    ASSERT_NE(layer->image(), nullptr);

    const auto px = layer->image()->getPixel(5, 5);
    EXPECT_NE(px, common::colors::Transparent);
}

static CanvasWidget* canvas(MainWindow& w)
{
    auto* c = w.findChild<CanvasWidget*>("canvas");
    EXPECT_NE(c, nullptr);
    return c;
}

TEST(UI_Zoom, ZoomActions_DoNothingWithoutImage)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    auto* c = canvas(w);
    ASSERT_NE(c, nullptr);

    // Pas de document => img_ null => hasImage() false
    const double before = c->scale();

    auto* zoomInAct = w.findChild<QAction*>("zoomInAct");
    auto* zoomOutAct = w.findChild<QAction*>("zoomOutAct");
    auto* resetZoomAct = w.findChild<QAction*>("resetZoomAct");
    ASSERT_NE(zoomInAct, nullptr);
    ASSERT_NE(zoomOutAct, nullptr);
    ASSERT_NE(resetZoomAct, nullptr);

    zoomInAct->trigger();
    zoomOutAct->trigger();
    resetZoomAct->trigger();
    pumpEvents();

    EXPECT_DOUBLE_EQ(c->scale(), before);
}

TEST(UI_Zoom, ZoomIn_IncreasesScale)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    // Crée un doc => refreshUIAfterDocChange met une image dans le canvas
    svc->newDocument({64, 64}, 72.f);
    pumpEvents();

    auto* c = canvas(w);
    ASSERT_NE(c, nullptr);

    c->setScale(1.0);
    pumpEvents();

    auto* zoomInAct = w.findChild<QAction*>("zoomInAct");
    ASSERT_NE(zoomInAct, nullptr);

    zoomInAct->trigger();
    pumpEvents();

    EXPECT_NEAR(c->scale(), 1.0 * 1.25, 1e-9);
}

TEST(UI_Zoom, ZoomOut_DecreasesScale)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);
    pumpEvents();

    auto* c = canvas(w);
    ASSERT_NE(c, nullptr);

    c->setScale(1.0);
    pumpEvents();

    auto* zoomOutAct = w.findChild<QAction*>("zoomOutAct");
    ASSERT_NE(zoomOutAct, nullptr);

    zoomOutAct->trigger();
    pumpEvents();

    EXPECT_NEAR(c->scale(), 1.0 / 1.25, 1e-9);
}

TEST(UI_Zoom, ResetZoom_SetsScaleTo1)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);
    pumpEvents();

    auto* c = canvas(w);
    ASSERT_NE(c, nullptr);

    c->setScale(2.5);
    pumpEvents();

    auto* resetZoomAct = w.findChild<QAction*>("resetZoomAct");
    ASSERT_NE(resetZoomAct, nullptr);

    resetZoomAct->trigger();
    pumpEvents();

    EXPECT_NEAR(c->scale(), 1.0, 1e-9);
}