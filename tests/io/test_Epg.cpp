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

TEST(EpgFormatTest, LoadTruncatedData)
{
    namespace fs = std::filesystem;
    const fs::path tmp = fs::temp_directory_path() / "epg_trunc.epg";

    // craft a header with a dataSize larger than the actual PNG bytes written
    {
        std::ofstream ofs(tmp, std::ios::binary);
        // MAGIC
        ofs.write("EPIGIMP", 7);
        // version
        int32_t version = 1;
        ofs.write(reinterpret_cast<const char*>(&version), sizeof(version));
        // w,h,c
        int32_t w = 2, h = 2, c = 4;
        ofs.write(reinterpret_cast<const char*>(&w), sizeof(w));
        ofs.write(reinterpret_cast<const char*>(&h), sizeof(h));
        ofs.write(reinterpret_cast<const char*>(&c), sizeof(c));
        // deliberately large data size
        int32_t dataSize = 1000000;
        ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        // write only a few bytes (not a valid PNG either)
        const char dummy[8] = {0, 1, 2, 3, 4, 5, 6, 7};
        ofs.write(dummy, sizeof(dummy));
    }

    ImageBuffer dst(1, 1);
    EXPECT_FALSE(EpgFormat::load(tmp.string(), dst));

    std::error_code ec;
    fs::remove(tmp, ec);
}

TEST(EpgFormatTest, LoadWrongVersion)
{
    namespace fs = std::filesystem;
    const fs::path tmp = fs::temp_directory_path() / "epg_wrong_version.epg";

    // write correct MAGIC but wrong version
    {
        std::ofstream ofs(tmp, std::ios::binary);
        ofs.write("EPIGIMP", 7);
        int32_t version = 2;  // unsupported
        ofs.write(reinterpret_cast<const char*>(&version), sizeof(version));
        int32_t w = 1, h = 1, c = 4;
        ofs.write(reinterpret_cast<const char*>(&w), sizeof(w));
        ofs.write(reinterpret_cast<const char*>(&h), sizeof(h));
        ofs.write(reinterpret_cast<const char*>(&c), sizeof(c));
        int32_t dataSize = 0;
        ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    }

    ImageBuffer dst(1, 1);
    EXPECT_FALSE(EpgFormat::load(tmp.string(), dst));

    std::error_code ec;
    fs::remove(tmp, ec);
}

TEST(EpgFormatTest, LoadBadHeader)
{
    namespace fs = std::filesystem;
    const fs::path tmp = fs::temp_directory_path() / "epg_bad_header.epg";

    // write only part of the header (truncated header)
    {
        std::ofstream ofs(tmp, std::ios::binary);
        // write only MAGIC and half of version
        ofs.write("EPIGIMP", 7);
        int16_t partial = 0x1234;
        ofs.write(reinterpret_cast<const char*>(&partial), sizeof(partial));
    }

    ImageBuffer dst(1, 1);
    EXPECT_FALSE(EpgFormat::load(tmp.string(), dst));

    std::error_code ec;
    fs::remove(tmp, ec);
}
