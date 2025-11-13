#pragma once
#include "window.hpp"

class ImageActions
{
   public:
    static void newImage(MainWindow* window);
    static void openImage(MainWindow* window);
    static void saveImage(MainWindow* window);
    static void closeImage(MainWindow* window);
};
