//
// Created by apolline on 23/02/2026.
//
#include <QApplication>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}