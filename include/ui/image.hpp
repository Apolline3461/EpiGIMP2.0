#pragma once
#include <QLabel>
#include <QPoint>
#include <QRect>

class MainWindow;
class QMouseEvent;
class QRubberBand;
class QPaintEvent;

class ImageActions
{
   public:
    static void newImage(MainWindow* window);
    static void openImage(MainWindow* window);
    static void saveImage(MainWindow* window);
    static void closeImage(MainWindow* window);
};
