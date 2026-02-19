#include <gtest/gtest.h>
#include <QApplication>
#include <QAction>
#include "ui/window.hpp"
#include "io/IStorage.hpp"

namespace
{
class DummyStorage final : public IStorage {
public:
    io::epg::OpenResult open(const std::string&) override { return {}; }
    void save(const Document&, const std::string&) override {}
    void exportImage(const Document&, const std::string&) override {}
};

static void ensureQtApp()
{
    if (QApplication::instance())
        return;

    int argc = 0;
    static char arg0[] = "test_ui";
    static char* argv[] = { arg0, nullptr };
    static QApplication app(argc, argv);
}

static QToolButton* btn(MainWindow& w, const char* name)
{
    auto* b = w.findChild<QToolButton*>(name);
    EXPECT_NE(b, nullptr) << "Missing button objectName=" << name;
    return b;
}

static QListWidget* layersList(MainWindow& w)
{
    // si tu as mis un objectName à la list c’est encore mieux,
    // sinon on la retrouve par type.
    auto* list = w.findChild<QListWidget*>();
    EXPECT_NE(list, nullptr) << "Could not find QListWidget (layers list)";
    return list;
}

TEST(MainWindow_CloseDocument, ResetsToolModesAndActions)
{
    ensureQtApp();

    auto storage = std::make_unique<DummyStorage>();
    app::AppService svc(std::move(storage));
    MainWindow w(svc);

    svc.newDocument({64, 64}, 72.f);

    auto* actBucket = w.findChild<QAction*>("act.bucket");
    auto* actSelect = w.findChild<QAction*>("act.select");
    auto* actColorPick   = w.findChild<QAction*>("act.colorPick");
    auto *actClose  = w.findChild<QAction*>("act.close");
    auto* actPicker = w.findChild<QAction*>("act.picker");


    ASSERT_NE(actBucket, nullptr);
    ASSERT_NE(actSelect, nullptr);
    ASSERT_NE(actColorPick, nullptr);
    ASSERT_NE(actPicker, nullptr);

    ASSERT_NE(actClose, nullptr);

    actBucket->setChecked(true);
    actSelect->setChecked(true);
    actPicker->setChecked(true);

    actClose->trigger();

    EXPECT_FALSE(actBucket->isChecked());
    EXPECT_FALSE(actSelect->isChecked());
    EXPECT_FALSE(actPicker->isChecked());
    EXPECT_FALSE(actColorPick->isEnabled());
}

TEST(LayersPanelButtons, NoDocument_AllDisabled)
{
    ensureQtApp();

    auto storage = std::make_unique<DummyStorage>();
    app::AppService svc(std::move(storage));
    MainWindow w(svc);

    // safety: ensure no doc
    svc.closeDocument();

    auto* add = btn(w, "btn.layer.add");
    auto* del = btn(w, "btn.layer.del");
    auto* up  = btn(w, "btn.layer.up");
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

    auto storage = std::make_unique<DummyStorage>();
    app::AppService svc(std::move(storage));
    MainWindow w(svc);

    svc.newDocument({64, 64}, 72.f);

    // force selection of background item in list (background is idx 0, but list is filled from top to bottom)
    auto* list = layersList(w);
    ASSERT_GT(list->count(), 0);

    // populateLayersList iterates i = count-1 .. 0, so background is last row.
    list->setCurrentRow(list->count() - 1);

    auto* add = btn(w, "btn.layer.add");
    auto* del = btn(w, "btn.layer.del");
    auto* up  = btn(w, "btn.layer.up");
    auto* down = btn(w, "btn.layer.down");
    auto* merge = btn(w, "btn.layer.mergeDown");

    EXPECT_TRUE(add->isEnabled());
    EXPECT_FALSE(del->isEnabled());
    EXPECT_FALSE(merge->isEnabled());

    // only background -> cannot go up (idx + 1 < n is false)
    EXPECT_FALSE(up->isEnabled());
    EXPECT_FALSE(down->isEnabled());
}

TEST(LayersPanelButtons, LockedLayerSelected_DeleteAndMergeDisabled)
{
    ensureQtApp();

    auto storage = std::make_unique<DummyStorage>();
    app::AppService svc(std::move(storage));
    MainWindow w(svc);

    svc.newDocument({64, 64}, 72.f);

    // add a layer
    app::LayerSpec spec{};
    spec.name = "L1";
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;
    spec.color = 0u;
    svc.addLayer(spec);

    // lock it
    svc.setLayerLocked(1, true);

    // select it in list:
    // list order = top is highest index. With 2 layers (0 bg, 1 L1):
    // row 0 -> layer 1, row 1 -> layer 0
    auto* list = layersList(w);
    ASSERT_EQ(list->count(), 2);
    list->setCurrentRow(0); // L1
    auto* del = btn(w, "btn.layer.del");
    auto* merge = btn(w, "btn.layer.mergeDown");

    EXPECT_FALSE(del->isEnabled());
    EXPECT_FALSE(merge->isEnabled());
}

}