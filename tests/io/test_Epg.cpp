/*
** EPITECH PROJECT, 2025
** EpiGIMP2.0
** File description:
** test_Epg
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include "io/EpgFormat.hpp"

TEST(EpgFormatTest, SaveAndLoad)
{
    namespace fs = std::filesystem;
    const fs::path tmp = fs::temp_directory_path() / "epg_test.epg";
    // create an image and set pixels to known values
    ImageBuffer src(3, 2);
    src.fill(0x11223344u);
    src.setPixel(1, 0, 0xFF0000FFu);  // red
    src.setPixel(2, 1, 0x00FF00FFu);  // green

    // save
    ASSERT_TRUE(EpgFormat::save(tmp.string(), src));

    // load into a different buffer
    ImageBuffer dst(1, 1);
    ASSERT_TRUE(EpgFormat::load(tmp.string(), dst));

    EXPECT_EQ(dst.width(), src.width());
    EXPECT_EQ(dst.height(), src.height());

    for (int y = 0; y < src.height(); ++y)
    {
        for (int x = 0; x < src.width(); ++x)
        {
            EXPECT_EQ(dst.getPixel(x, y), src.getPixel(x, y));
        }
    }

    std::error_code ec;
    fs::remove(tmp, ec);
}

TEST(EpgFormatTest, LoadCorruptedFile)
{
    namespace fs = std::filesystem;
    const fs::path tmp = fs::temp_directory_path() / "epg_corrupt.epg";

    // write some invalid data
    {
        std::ofstream ofs(tmp, std::ios::binary);
        ofs << "NOTEPG";
    }

    ImageBuffer dst(1, 1);
    EXPECT_FALSE(EpgFormat::load(tmp.string(), dst));

    std::error_code ec;
    fs::remove(tmp, ec);
}