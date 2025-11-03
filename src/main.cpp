#include "main.hpp"
#include <QApplication>
#include <QMainWindow>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Create a main window
    QMainWindow window;
    window.setWindowTitle("EPIGIMP2.0");

    // Add a simple label as central widget
    QLabel *label = new QLabel("Hello, Qt World!");
    label->setAlignment(Qt::AlignCenter);
    window.setCentralWidget(label);

    // Resize and show the window
    window.resize(400, 300);
    window.show();

    // Run the Qt event loop
    return app.exec();
}
