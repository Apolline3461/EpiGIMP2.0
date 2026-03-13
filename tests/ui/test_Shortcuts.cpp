//
// Created by apolline on 10/03/2026.
//
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QKeySequence>
#include <QTest>
#include <QTextEdit>
#include <QTimer>

#include "ui/window.hpp"
#include "ui_test_helpers.hpp"

#include <core/ImageBuffer.hpp>
#include <core/Layer.hpp>
#include <gtest/gtest.h>

using namespace ui_test;

TEST(MainWindow_Shortcuts, CopySelection_AddsLayer_NoPopup)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    svc->newDocument({32, 32}, 72.f);

    // create a selection
    svc->setSelectionRect({1, 1, 4, 4});

    auto* actCopy = w.findChild<QAction*>("copySelectionAct");
    ASSERT_NE(actCopy, nullptr);

    const int before = static_cast<int>(svc->document().layerCount());
    actCopy->trigger();
    const int after = static_cast<int>(svc->document().layerCount());
    EXPECT_EQ(after, before + 1);

    // Ensure no modal QDialog is currently visible
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget* wgt : widgets)
    {
        auto* dlg = qobject_cast<QDialog*>(wgt);
        if (!dlg)
            continue;
        EXPECT_FALSE(dlg->isVisible()) << "Unexpected visible dialog opened by shortcut";
    }
}

static QAction* findAction(MainWindow& w, const char* name)
{
    auto* a = w.findChild<QAction*>(name);
    return a;
}

TEST(UI_Shortcuts, CtrlSlash_TriggersHelpDialogAndCloses)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    // Auto-ferme le dialog modal dès qu'il apparaît
    QTimer::singleShot(
        0,
        []()
        {
            if (auto* dlg = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                dlg->accept();
        });

    auto* helpAct = findAction(w, "act.shortcutsHelp");
    ASSERT_NE(helpAct, nullptr);

    // Envoie Ctrl+/ au niveau app (ApplicationShortcut)
    QTest::keyClick(QApplication::focusWidget() ? QApplication::focusWidget() : &w, Qt::Key_Slash,
                    Qt::ControlModifier);

    pumpEvents();

    // Vérifie qu'on n'est pas resté bloqué dans une modal
    EXPECT_EQ(QApplication::activeModalWidget(), nullptr);
}

TEST(UI_Shortcuts, F1_TriggersHelpDialogAndCloses)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    QTimer::singleShot(
        0,
        []()
        {
            if (auto* dlg = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                dlg->accept();
        });

    auto* helpAct = findAction(w, "act.shortcutsHelp");
    ASSERT_NE(helpAct, nullptr);

    QTest::keyClick(&w, Qt::Key_F1);
    pumpEvents();

    EXPECT_EQ(QApplication::activeModalWidget(), nullptr);
}

TEST(UI_HelpDialog, ContainsSomeKnownShortcuts)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    auto* helpAct = w.findChild<QAction*>("act.shortcutsHelp");
    ASSERT_NE(helpAct, nullptr);

    // On ouvre via trigger direct (pas le clavier) pour tester le contenu
    QTimer::singleShot(0,
                       []()
                       {
                           auto* dlg = qobject_cast<QDialog*>(QApplication::activeModalWidget());
                           ASSERT_NE(dlg, nullptr);

                           auto* te = dlg->findChild<QTextEdit*>();
                           ASSERT_NE(te, nullptr);

                           const QString txt = te->toPlainText();

                           EXPECT_TRUE(txt.contains("FICHIER") || txt.contains("Fichier"));
                           EXPECT_TRUE(txt.contains("Outils") || txt.contains("OUTILS"));
                           EXPECT_TRUE(txt.contains("Pinceau"));

                           dlg->accept();
                       });

    helpAct->trigger();
    QCoreApplication::processEvents();
}

TEST(UI_LayersShortcuts, CtrlL_TogglesLockOnSelectedLayer)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);
    app::LayerSpec spec{};
    spec.name = "L1";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0u;
    svc->addLayer(spec);
    QCoreApplication::processEvents();

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    ASSERT_GE(list->count(), 1);

    list->setCurrentRow(0);
    QCoreApplication::processEvents();

    ASSERT_FALSE(svc->document().layerAt(1)->locked());

    QTest::keyClick(&w, Qt::Key_L, Qt::ControlModifier);
    QCoreApplication::processEvents();

    EXPECT_TRUE(svc->document().layerAt(1)->locked());
}

TEST(UI_LayersShortcuts, CtrlH_TogglesVisibilityOnSelectedLayer)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);
    app::LayerSpec spec{};
    spec.name = "L1";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0u;
    svc->addLayer(spec);
    QCoreApplication::processEvents();

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    list->setCurrentRow(0);
    QCoreApplication::processEvents();

    ASSERT_TRUE(svc->document().layerAt(1)->visible());

    QTest::keyClick(&w, Qt::Key_H, Qt::ControlModifier);
    QCoreApplication::processEvents();

    EXPECT_FALSE(svc->document().layerAt(1)->visible());
}

TEST(UI_LayersShortcuts, Delete_RemovesSelectedLayerIfUnlocked)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);
    app::LayerSpec spec{};
    spec.name = "L1";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0u;
    svc->addLayer(spec);
    QCoreApplication::processEvents();

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    list->setCurrentRow(0);
    QCoreApplication::processEvents();

    ASSERT_EQ(svc->document().layerCount(), 2u);

    QTest::keyClick(&w, Qt::Key_Delete);
    QCoreApplication::processEvents();

    EXPECT_EQ(svc->document().layerCount(), 1u);
}

TEST(UI_LayersShortcuts, CtrlE_MergeDown_ReducesLayerCount)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);

    app::LayerSpec spec{};
    spec.name = "L1";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0u;
    svc->addLayer(spec);
    spec.name = "L2";
    svc->addLayer(spec);

    QCoreApplication::processEvents();

    ASSERT_EQ(svc->document().layerCount(), 3u);

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    list->setCurrentRow(0);  // top layer
    QCoreApplication::processEvents();

    QTest::keyClick(&w, Qt::Key_E, Qt::ControlModifier);
    QCoreApplication::processEvents();

    EXPECT_EQ(svc->document().layerCount(), 2u);
}

// -----------------------------------------------------------------------------
// Ctrl+J : Duplicate Layer
// -----------------------------------------------------------------------------

TEST(UI_LayersShortcuts, CtrlJ_Duplicates_NormalLayer_IncreasesCount)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);

    app::LayerSpec spec{};
    spec.name = "L1";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0xFF00FF00u;
    svc->addLayer(spec);

    pumpEvents();

    // layers: [0]=BG, [1]=L1
    ASSERT_EQ(svc->document().layerCount(), 2u);

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->count(), 2);

    // List order is top->bottom (idx max first), so row 0 == L1
    list->setCurrentRow(0);
    pumpEvents();

    const auto beforeCount = svc->document().layerCount();
    QTest::keyClick(&w, Qt::Key_J, Qt::ControlModifier);
    pumpEvents();

    EXPECT_EQ(svc->document().layerCount(), beforeCount + 1);

    // With "insertAt = idx + 1", duplication of idx=1 becomes idx=2
    auto duplicated = svc->document().layerAt(2);
    ASSERT_NE(duplicated, nullptr);
    EXPECT_TRUE(duplicated->image() != nullptr);
}

TEST(UI_LayersShortcuts, CtrlJ_Duplicates_LockedLayer_CopyIsLocked)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);

    app::LayerSpec spec{};
    spec.name = "Locked";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0u;
    svc->addLayer(spec);

    // lock the layer (idx 1)
    svc->setLayerLocked(1, true);
    pumpEvents();

    ASSERT_EQ(svc->document().layerCount(), 2u);
    ASSERT_TRUE(svc->document().layerAt(1)->locked());

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->count(), 2);

    // select top layer (idx 1)
    list->setCurrentRow(0);
    pumpEvents();

    QTest::keyClick(&w, Qt::Key_J, Qt::ControlModifier);
    pumpEvents();

    ASSERT_EQ(svc->document().layerCount(), 3u);

    // duplicated is inserted above => idx 2
    auto dup = svc->document().layerAt(2);
    ASSERT_NE(dup, nullptr);
    EXPECT_TRUE(dup->locked()) << "Duplicated layer should keep locked state";
}

/*TEST(UI_LayersShortcuts, CtrlJ_Duplicates_Background_CopyIsUnlocked)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);

    showAndActivate(w);

    svc->newDocument({64, 64}, 72.f);
    pumpEvents();

    // Only background exists initially
    ASSERT_EQ(svc->document().layerCount(), 1u);

    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->count(), 1);

    // Background is last row (also row 0 since single item)
    list->setCurrentRow(list->count() - 1);
    pumpEvents();

    QTest::keyClick(&w, Qt::Key_J, Qt::ControlModifier);
    pumpEvents();

    ASSERT_EQ(svc->document().layerCount(), 2u);

    // If background duplicated with insertAt = 1, new layer is idx 1
    auto dup = svc->document().layerAt(1);
    ASSERT_NE(dup, nullptr);

    EXPECT_FALSE(dup->locked())
        << "Duplicated background should be unlocked (Photoshop-like behavior)";
}*/

TEST(UI_LayersShortcuts, CtrlJ_SetsActiveLayerToDuplicated)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    svc->newDocument({8, 8}, 72.f);

    app::LayerSpec spec{};
    spec.name="L1"; spec.visible=true; spec.locked=false; spec.opacity=1.f; spec.color=0u;
    svc->addLayer(spec);
    pumpEvents();

    // select top layer (L1)
    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    list->setCurrentRow(0);
    pumpEvents();

    QTest::keyClick(&w, Qt::Key_J, Qt::ControlModifier);
    pumpEvents();

    ASSERT_EQ(svc->document().layerCount(), 3u);
    EXPECT_EQ(svc->activeLayer(), 2u) << "Duplicated layer should become active (insertAt = idx+1)";
}

TEST(UI_LayersShortcuts, CtrlJ_DuplicateIsDeepCopy_ImageBufferNotShared)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    svc->newDocument({8, 8}, 72.f);

    app::LayerSpec spec{};
    spec.name="L1"; spec.visible=true; spec.locked=false; spec.opacity=1.f; spec.color=0u;
    svc->addLayer(spec);
    pumpEvents();

    // Put a known pixel in original (idx 1)
    auto orig = svc->document().layerAt(1);
    ASSERT_NE(orig, nullptr);
    ASSERT_NE(orig->image(), nullptr);
    orig->image()->setPixel(0, 0, 0xFF112233u);

    // select top layer (row 0 -> idx 1)
    auto* list = layersList(w);
    ASSERT_NE(list, nullptr);
    list->setCurrentRow(0);
    pumpEvents();

    QTest::keyClick(&w, Qt::Key_J, Qt::ControlModifier);
    pumpEvents();

    auto dup = svc->document().layerAt(2);
    ASSERT_NE(dup, nullptr);
    ASSERT_NE(dup->image(), nullptr);

    // same initial pixel copied
    EXPECT_EQ(dup->image()->getPixel(0,0), 0xFF112233u);

    // mutate original, duplicate must not change
    orig->image()->setPixel(0, 0, 0xFFABCDEFu);
    EXPECT_EQ(dup->image()->getPixel(0,0), 0xFF112233u);
}

TEST(UI_Tools, CtrlM_TogglesMoveLayerAction)
{
    ensureQtApp();
    auto svc = makeService();
    MainWindow w(*svc);
    showAndActivate(w);

    svc->newDocument({64,64}, 72.f);
    pumpEvents();

    auto* actMove = w.findChild<QAction*>("act.moveLayer");
    ASSERT_NE(actMove, nullptr);

    QTest::keyClick(&w, Qt::Key_M, Qt::ControlModifier);
    pumpEvents();
    EXPECT_TRUE(actMove->isChecked());

    QTest::keyClick(&w, Qt::Key_P);
    pumpEvents();
    EXPECT_FALSE(actMove->isChecked());
}