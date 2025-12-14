/*
** Consolidated EPG tests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "core/document.hpp"
#include "core/ImageBuffer.h"
#include "core/Layer.h"
#include "io/EpgFormat.hpp"
#include "stb_image.h"

using namespace std;

// From test_Epg.cpp
TEST(EpgFormat, SaveAndOpenRoundTrip)
{
    Document doc;
    doc.width = 16;
    doc.height = 16;
    doc.dpi = 72.0f;

    auto buf = make_shared<ImageBuffer>(16, 16);
    buf->fill(0xFF0000FFu);  // rouge opaque

    auto layer = make_shared<Layer>(1ULL, string("Layer1"), buf, true, false, 1.0f);
    doc.layers.push_back(layer);

    ZipEpgStorage storage;

    auto tmp = std::filesystem::temp_directory_path() / "epg_test_roundtrip.epg";
    std::string path = tmp.string();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    EXPECT_NO_THROW(storage.save(doc, path));

    OpenResult res = storage.open(path);
    EXPECT_TRUE(res.success) << res.errorMessage;
    ASSERT_NE(res.document, nullptr);
    EXPECT_EQ(res.document->width, doc.width);
    EXPECT_EQ(res.document->height, doc.height);
    ASSERT_EQ(res.document->layers.size(), 1u);

    auto loadedLayer = res.document->layers[0];
    EXPECT_EQ(loadedLayer->name(), "Layer1");
    ASSERT_NE(loadedLayer->image(), nullptr);
    uint32_t px = loadedLayer->image()->getPixel(0, 0);
    EXPECT_EQ(px, buf->getPixel(0, 0));

    std::filesystem::remove(path, ec);
}

TEST(EpgFormat, ExportPngCreatesFile)
{
    Document doc;
    doc.width = 8;
    doc.height = 8;

    auto buf = make_shared<ImageBuffer>(8, 8);
    buf->fill(0x00FF00FFu);  // vert opaque

    auto layer = make_shared<Layer>(1ULL, string("L"), buf, true, false, 1.0f);
    doc.layers.push_back(layer);

    ZipEpgStorage storage;

    auto tmp = std::filesystem::temp_directory_path() / "epg_export_test.png";
    std::string path = tmp.string();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    EXPECT_NO_THROW(storage.exportPng(doc, path));
    EXPECT_TRUE(std::filesystem::exists(path));

    std::ifstream f(path, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    unsigned char sig[8];
    f.read(reinterpret_cast<char*>(sig), 8);
    ASSERT_EQ(f.gcount(), 8);
    EXPECT_EQ(sig[0], 137);
    EXPECT_EQ(sig[1], 80);
    EXPECT_EQ(sig[2], 78);
    EXPECT_EQ(sig[3], 71);
    EXPECT_EQ(sig[4], 13);
    EXPECT_EQ(sig[5], 10);
    EXPECT_EQ(sig[6], 26);
    EXPECT_EQ(sig[7], 10);
    f.close();
    std::filesystem::remove(path, ec);
}

// From test_Epg_extra.cpp
TEST(EpgFormatExtra, OpenNonExistentFile)
{
    ZipEpgStorage storage;
    auto tmp = std::filesystem::temp_directory_path() / "epg_nonexistent.epg";
    std::string path = tmp.string();

    // ensure file does not exist
    std::error_code ec;
    std::filesystem::remove(path, ec);

    OpenResult res = storage.open(path);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.errorMessage.size(), 0u);
}

TEST(EpgFormatExtra, SaveAndOpenMultipleLayers)
{
    Document doc;
    doc.width = 4;
    doc.height = 4;

    auto buf1 = make_shared<ImageBuffer>(4, 4);
    buf1->fill(0x112233FFu);
    auto buf2 = make_shared<ImageBuffer>(4, 4);
    buf2->fill(0x445566FFu);

    auto l1 = make_shared<Layer>(1ULL, string("L1"), buf1);
    auto l2 = make_shared<Layer>(2ULL, string("L2"), buf2);
    doc.layers.push_back(l1);
    doc.layers.push_back(l2);

    ZipEpgStorage storage;
    auto tmp = std::filesystem::temp_directory_path() / "epg_test_multilayer.epg";
    std::string path = tmp.string();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    EXPECT_NO_THROW(storage.save(doc, path));

    OpenResult res = storage.open(path);
    EXPECT_TRUE(res.success) << res.errorMessage;
    ASSERT_NE(res.document, nullptr);
    EXPECT_EQ(res.document->layers.size(), 2u);

    // verify pixel equality for both layers
    EXPECT_EQ(res.document->layers[0]->image()->getPixel(0, 0), buf1->getPixel(0, 0));
    EXPECT_EQ(res.document->layers[1]->image()->getPixel(0, 0), buf2->getPixel(0, 0));

    std::filesystem::remove(path, ec);
}

TEST(EpgFormatExtra, SaveEmptyDocument)
{
    Document doc;
    doc.width = 2;
    doc.height = 2;

    ZipEpgStorage storage;
    auto tmp = std::filesystem::temp_directory_path() / "epg_test_empty.epg";
    std::string path = tmp.string();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    // Saving an empty document should not throw and should be openable
    EXPECT_NO_THROW(storage.save(doc, path));
    OpenResult res = storage.open(path);
    EXPECT_TRUE(res.success) << res.errorMessage;
    ASSERT_NE(res.document, nullptr);
    EXPECT_EQ(res.document->layers.size(), 0u);

    std::filesystem::remove(path, ec);
}

// From test_Epg_more.cpp
TEST(EpgFormatMore, SaveAndOpenPreserveLayerProperties)
{
    Document doc;
    doc.width = 8;
    doc.height = 8;
    doc.dpi = 300.0f;

    auto buf = make_shared<ImageBuffer>(8, 8);
    buf->fill(0xAABBCCFFu);

    // create layer with various properties
    auto layer = make_shared<Layer>(42ULL, string("Props"), buf, false, true, 0.5f);
    doc.layers.push_back(layer);

    ZipEpgStorage storage;
    auto tmp = std::filesystem::temp_directory_path() / "epg_test_props.epg";
    std::string path = tmp.string();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    EXPECT_NO_THROW(storage.save(doc, path));

    OpenResult res = storage.open(path);
    EXPECT_TRUE(res.success) << res.errorMessage;
    ASSERT_NE(res.document, nullptr);

    // dpi saved as integer in manifest -> compare as int
    EXPECT_EQ(static_cast<int>(res.document->dpi), static_cast<int>(doc.dpi));

    ASSERT_EQ(res.document->layers.size(), 1u);
    auto loaded = res.document->layers[0];
    // Epg format génère des IDs séquentiels (1-based) lors de la sauvegarde,
    // donc l'ID rétabli après ouverture doit être 1 pour le premier calque.
    EXPECT_EQ(loaded->id(), 1ULL);
    EXPECT_EQ(loaded->name(), "Props");
    EXPECT_EQ(loaded->visible(), false);
    EXPECT_EQ(loaded->locked(), true);
    EXPECT_FLOAT_EQ(loaded->opacity(), 0.5f);

    std::filesystem::remove(path, ec);
}

TEST(EpgFormatMore, ExportPng_CompositionWithOpacity)
{
    Document doc;
    doc.width = 4;
    doc.height = 4;

    // bottom: green opaque
    auto bottom = make_shared<ImageBuffer>(4, 4);
    bottom->fill(0x00FF00FFu);
    // top: red opaque but with opacity 0.5 on layer
    auto top = make_shared<ImageBuffer>(4, 4);
    top->fill(0xFF0000FFu);

    auto l1 = make_shared<Layer>(1ULL, string("BG"), bottom, true, false, 1.0f);
    auto l2 = make_shared<Layer>(2ULL, string("Top"), top, true, false, 0.5f);
    doc.layers.push_back(l1);
    doc.layers.push_back(l2);

    ZipEpgStorage storage;
    auto tmp = std::filesystem::temp_directory_path() / "epg_comp_test.png";
    std::string path = tmp.string();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    EXPECT_NO_THROW(storage.exportPng(doc, path));
    EXPECT_TRUE(std::filesystem::exists(path));

    int w, h, channels;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    ASSERT_NE(data, nullptr);
    ASSERT_EQ(w, doc.width);
    ASSERT_EQ(h, doc.height);
    ASSERT_EQ(channels, 4);

    // pixel at 0,0 -> composition result: (127,127,0,255)
    int idx = 0;
    unsigned char r = data[idx + 0];
    unsigned char g = data[idx + 1];
    unsigned char b = data[idx + 2];
    unsigned char a = data[idx + 3];

    EXPECT_EQ(r, 127);
    EXPECT_EQ(g, 127);
    EXPECT_EQ(b, 0);
    EXPECT_EQ(a, 255);

    stbi_image_free(data);
    std::filesystem::remove(path, ec);
}

TEST(EpgFormatMore, ExportPng_ThrowsOnEmptyDocument)
{
    Document doc;
    doc.width = 4;
    doc.height = 4;

    ZipEpgStorage storage;
    EXPECT_THROW(storage.exportPng(doc, "some_path.png"), std::runtime_error);
}


TEST(EpgFormatRAII, SaveCreatesPreviewAndLayerEntries)
{
    Document doc;
    doc.width = 16;
    doc.height = 16;

    auto buf = make_shared<ImageBuffer>(16, 16);
    buf->fill(0x123456FFu);

    auto layer = make_shared<Layer>(1ULL, string("Layer1"), buf, true, false, 1.0f);
    doc.layers.push_back(layer);

    ZipEpgStorage storage;

    auto tmp = std::filesystem::temp_directory_path() / "epg_test_raii.epg";
    std::string path = tmp.string();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    EXPECT_NO_THROW(storage.save(doc, path));

    int err = 0;
    zip_t* z = zip_open(path.c_str(), ZIP_RDONLY, &err);
    ASSERT_NE(z, nullptr);

    zip_int64_t idxPreview = zip_name_locate(z, "preview.png", 0);
    zip_int64_t idxLayer = zip_name_locate(z, "layers/0001.png", 0);

    EXPECT_GE(idxPreview, 0);
    EXPECT_GE(idxLayer, 0);

    zip_close(z);

    std::filesystem::remove(path, ec);
}

TEST(EpgHelpers, ComposePreviewAndEncodeProducesPngSignature)
{
    Document doc;
    doc.width = 16;
    doc.height = 8;

    auto buf = make_shared<ImageBuffer>(16, 8);
    buf->fill(0x112233FFu);

    auto layer = make_shared<Layer>(1ULL, string("L"), buf, true, false, 1.0f);
    doc.layers.push_back(layer);

    ZipEpgStorage storage;

    int w = 0, h = 0;
    auto preview = storage.composePreviewRGBA(doc, w, h);
    ASSERT_FALSE(preview.empty());
    ASSERT_GT(w, 0);
    ASSERT_GT(h, 0);

    auto png = storage.encodePngToVector(preview.data(), w, h, 4, w * 4);
    ASSERT_GE(png.size(), 8u);

    unsigned char expected[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(png[i], expected[i]);
}

TEST(EpgHelpers, WritePreviewToZipCreatesEntry)
{
    Document doc;
    doc.width = 10;
    doc.height = 10;
    auto buf = make_shared<ImageBuffer>(10, 10);
    buf->fill(0xFFFFFFFFu);
    doc.layers.push_back(make_shared<Layer>(1ULL, string("L"), buf, true, false, 1.0f));

    ZipEpgStorage storage;
    int w = 0, h = 0;
    auto preview = storage.composePreviewRGBA(doc, w, h);
    auto png = storage.encodePngToVector(preview.data(), w, h, 4, w * 4);

    auto tmp = std::filesystem::temp_directory_path() / "epg_helpers_preview.epg";
    std::string path = tmp.string();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    int err = 0;
    zip_t* z = zip_open(path.c_str(), ZIP_TRUNCATE | ZIP_CREATE, &err);
    ASSERT_NE(z, nullptr);

    // write preview
    EXPECT_NO_THROW(storage.writePreviewToZip(z, png));

    zip_close(z);

    // reopen and check
    int err2 = 0;
    zip_t* zr = zip_open(path.c_str(), ZIP_RDONLY, &err2);
    ASSERT_NE(zr, nullptr);
    zip_int64_t idx = zip_name_locate(zr, "preview.png", 0);
    EXPECT_GE(idx, 0);
    zip_close(zr);

    std::filesystem::remove(path, ec);
}

TEST(EpgHelpers, ComposeFlattenedRGBAMatchesExpectedComposition)
{
    Document doc;
    doc.width = 4;
    doc.height = 4;

    auto bottom = make_shared<ImageBuffer>(4, 4);
    bottom->fill(0x00FF00FFu);
    auto top = make_shared<ImageBuffer>(4, 4);
    top->fill(0xFF0000FFu);

    auto l1 = make_shared<Layer>(1ULL, string("BG"), bottom, true, false, 1.0f);
    auto l2 = make_shared<Layer>(2ULL, string("Top"), top, true, false, 0.5f);
    doc.layers.push_back(l1);
    doc.layers.push_back(l2);

    ZipEpgStorage storage;
    auto out = storage.composeFlattenedRGBA(doc);
    ASSERT_EQ(static_cast<int>(out.size()), doc.width * doc.height * 4);

    unsigned char r = out[0];
    unsigned char g = out[1];
    unsigned char b = out[2];
    unsigned char a = out[3];

    EXPECT_EQ(r, 127);
    EXPECT_EQ(g, 127);
    EXPECT_EQ(b, 0);
    EXPECT_EQ(a, 255);
}

TEST(EpgHelpers, WriteAndLoadManifestAndLayers)
{
    Document doc;
    doc.width = 8;
    doc.height = 8;
    auto buf = make_shared<ImageBuffer>(8, 8);
    buf->fill(0x123456FFu);
    doc.layers.push_back(make_shared<Layer>(1ULL, string("L1"), buf, true, false, 1.0f));

    ZipEpgStorage storage;
    Manifest m = storage.createManifestFromDocument(doc);

    auto tmp = std::filesystem::temp_directory_path() / "epg_helpers_manifest.epg";
    std::string path = tmp.string();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    int err = 0;
    zip_t* z = zip_open(path.c_str(), ZIP_TRUNCATE | ZIP_CREATE, &err);
    ASSERT_NE(z, nullptr);

    // write layers and manifest
    EXPECT_NO_THROW(storage.writeLayersToZip(z, m, doc));
    EXPECT_NO_THROW(storage.writeManifestToZip(z, m));

    zip_close(z);

    // reopen and load via storage helper
    int err2 = 0;
    zip_t* zr = zip_open(path.c_str(), ZIP_RDONLY, &err2);
    ASSERT_NE(zr, nullptr);

    Manifest loaded = storage.loadManifestFromZip(zr);
    EXPECT_EQ(loaded.layers.size(), m.layers.size());
    for (const auto& L : loaded.layers)
    {
        EXPECT_FALSE(L.sha256.empty());
    }

    // ensure layer file exists in zip
    zip_int64_t idxLayer = zip_name_locate(zr, "layers/0001.png", 0);
    EXPECT_GE(idxLayer, 0);

    zip_close(zr);
    std::filesystem::remove(path, ec);
}
