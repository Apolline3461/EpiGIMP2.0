//
// Created by apolline on 11/03/2026.
//

#pragma once

#include <QTest>
#include <QApplication>
#include <QCoreApplication>
#include <QListWidget>
#include <memory>

#include "app/AppService.hpp"
#include "io/IStorage.hpp"

namespace ui_test {

class DummyStorage final : public IStorage {
public:
    io::epg::OpenResult open(const std::string&) override { return {}; }
    void save(const Document&, const std::string&) override {}
    void exportImage(const Document&, const std::string&) override {}
};

inline void ensureQtApp()
{
    if (QApplication::instance())
        return;

    int argc = 0;
    static char arg0[] = "test_ui";
    static char* argv[] = { arg0, nullptr };
    static QApplication app(argc, argv);
}

inline std::unique_ptr<app::AppService> makeService()
{
    auto storage = std::make_unique<DummyStorage>();
    return std::make_unique<app::AppService>(std::move(storage));
}

inline void pumpEvents()
{
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
}

inline QListWidget* layersList(QWidget& w)
{
    if (auto* list = w.findChild<QListWidget*>("layerList"))
        return list;
    if (auto* list = w.findChild<QListWidget*>("layersList"))
        return list;

    // fallback si objectName absent
    auto lists = w.findChildren<QListWidget*>();
    if (!lists.isEmpty())
        return lists.first();

    return nullptr;
}

static void showAndActivate(QWidget& w)
{
    w.show();
    QTest::qWait(10);
    QCoreApplication::processEvents();
}

} // namespace ui_test
