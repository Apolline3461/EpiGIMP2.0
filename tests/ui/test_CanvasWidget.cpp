//
// Created by apolline on 12/03/2026.
//

#include <QTest>
#include <gtest/gtest.h>

#include "ui/CanvasWidget.hpp"
#include "ui_test_helpers.hpp"

using namespace ui_test;

TEST(CanvasWidget, Scale_IsClamped)
{
    ensureQtApp();
    CanvasWidget w;
    w.resize(200, 200);
    w.setImage(QImage(64, 64, QImage::Format_ARGB32));
    showAndActivate(w);

    w.setScale(0.001);
    EXPECT_GE(w.scale(), 0.10);

    w.setScale(999.0);
    EXPECT_LE(w.scale(), 8.0);
}

TEST(CanvasWidget, Pan_IsClamped)
{
    ensureQtApp();
    CanvasWidget w;
    w.resize(200, 200);
    w.setImage(QImage(64, 64, QImage::Format_ARGB32));
    showAndActivate(w);

    // Pan énorme -> doit être clampé
    w.setPan(QPointF(10000, 10000));
    auto p1 = w.pan();
    EXPECT_LT(p1.x(), 10000);
    EXPECT_LT(p1.y(), 10000);

    // Pan négatif énorme -> clampé aussi
    w.setPan(QPointF(-10000, -10000));
    auto p2 = w.pan();
    EXPECT_GT(p2.x(), -10000);
    EXPECT_GT(p2.y(), -10000);
}

TEST(CanvasWidget, DocScreen_TransformsRoundTrip)
{
    ensureQtApp();
    CanvasWidget w;
    w.resize(200, 200);
    w.setImage(QImage(100, 100, QImage::Format_ARGB32));
    showAndActivate(w);

    w.setScale(2.0);
    w.setPan(QPointF(10, 20));

    common::Point p{5, 7};
    QPoint sp = w.docToScreen(p);
    common::Point back = w.screenToDoc(sp);

    EXPECT_EQ(back.x, p.x);
    EXPECT_EQ(back.y, p.y);
}

TEST(CanvasWidget, DragPreview_SetPosClear)
{
    ensureQtApp();
    CanvasWidget w;
    w.resize(200, 200);
    w.setImage(QImage(64, 64, QImage::Format_ARGB32));
    showAndActivate(w);

    QImage layer(16, 16, QImage::Format_ARGB32);
    w.setDragLayerPreview(layer, 3, 4);
    pumpEvents();

    w.setDragLayerPos(10, 12);
    pumpEvents();

    w.clearDragLayerPreview();
    pumpEvents();

    SUCCEED(); // couvre les branches + update()
}

TEST(CanvasWidget, Selection_EmitsSelectionFinishedDoc)
{
    ensureQtApp();
    CanvasWidget w;
    w.resize(200, 200);
    w.setImage(QImage(64, 64, QImage::Format_ARGB32));
    showAndActivate(w);

    w.setSelectionEnable(true);

    common::Rect out{};
    bool got = false;
    QObject::connect(&w, &CanvasWidget::selectionFinishedDoc, [&](common::Rect r) {
        out = r; got = true;
    });

    // drag selection from (10,10) to (30,40)
    QTest::mousePress(&w, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
    QTest::mouseMove(&w, QPoint(30,40));
    QTest::mouseRelease(&w, Qt::LeftButton, Qt::NoModifier, QPoint(30,40));
    pumpEvents();

    EXPECT_TRUE(got);
    EXPECT_GT(out.w, 0);
    EXPECT_GT(out.h, 0);
}

TEST(CanvasWidget, Pencil_Stroke_EmitsBeginMoveEnd)
{
    ensureQtApp();
    CanvasWidget w;
    w.resize(200, 200);
    w.setImage(QImage(64, 64, QImage::Format_ARGB32));
    showAndActivate(w);

    w.setPencilEnable(true);

    bool began = false, moved = false, ended = false;
    QObject::connect(&w, &CanvasWidget::beginStroke, [&](common::Point){ began = true; });
    QObject::connect(&w, &CanvasWidget::moveStroke,  [&](common::Point){ moved = true; });
    QObject::connect(&w, &CanvasWidget::endStroke,   [&](){ ended = true; });

    QTest::mousePress(&w, Qt::LeftButton, Qt::NoModifier, QPoint(5,5));
    QTest::mouseMove(&w, QPoint(10,10));
    QTest::mouseRelease(&w, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
    pumpEvents();

    EXPECT_TRUE(began);
    EXPECT_TRUE(moved);
    EXPECT_TRUE(ended);
}
