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
    actColorPick->setChecked(true);

    actClose->trigger();

    EXPECT_FALSE(actBucket->isChecked());
    EXPECT_FALSE(actSelect->isChecked());
    EXPECT_FALSE(actPicker->isChecked());
    EXPECT_FALSE(actColorPick->isChecked());
}
}