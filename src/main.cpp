#include <QApplication>

#include "window.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Configuration de l'application
    QApplication::setApplicationName("EpiGimp 2.0");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("Epitech");

    MainWindow window;
    window.show();

    return app.exec();
}
