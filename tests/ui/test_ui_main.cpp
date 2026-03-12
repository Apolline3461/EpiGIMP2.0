//
// Created by apolline on 23/02/2026.
//
#include <QApplication>
#include <gtest/gtest.h>

static void setQtOffscreen()
{
#if defined(_WIN32)
    // MSVC: setenv() n'existe pas
    _putenv_s("QT_QPA_PLATFORM", "offscreen");
#else
    setenv("QT_QPA_PLATFORM", "offscreen", 1);
#endif
}

int main(int argc, char** argv)
{

    setQtOffscreen();
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}