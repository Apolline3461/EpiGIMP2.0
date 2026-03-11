#include <gtest/gtest.h>

#include <QAction>
#include <QListWidget>
#include <QTest>

#include "ui/window.hpp"
#include "ui_test_helpers.hpp"

using namespace ui_test;

TEST(UI_Focus, F6_FocusesCanvas)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    auto* canvas = w.findChild<QWidget*>("canvas");
    ASSERT_NE(canvas, nullptr);

    QTest::keyClick(&w, Qt::Key_F6);
    QCoreApplication::processEvents();

    EXPECT_TRUE(canvas->hasFocus());
}

TEST(UI_Focus, F7_FocusesLayersList)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);

    QTest::keyClick(&w, Qt::Key_F7);
    QCoreApplication::processEvents();

    EXPECT_TRUE(list->hasFocus());
}