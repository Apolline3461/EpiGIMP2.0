#include <QApplication>

#include "app/AppService.hpp"
#include "io/EpgFormat.hpp"
#include "ui/window.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Configuration de l'application
    QApplication::setApplicationName("EpiGimp 2.0");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("Epitech");

    auto storage = std::make_unique<ZipEpgStorage>();
    app::AppService svc(std::move(storage));

    MainWindow window(svc);
    window.show();

    return app.exec();
}
