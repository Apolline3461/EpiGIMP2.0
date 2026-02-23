//
// Created by apolline on 20/02/2026.
//

#include <gtest/gtest.h>
#include <filesystem>

#include "app/AppService.hpp"
#include "io/EpgFormat.hpp"
#include "common/Colors.hpp"

TEST(AppService_EPG, SaveOpen_RoundTrip_PreservesLayerCount)
{
    auto tmp = std::filesystem::temp_directory_path() / "appservice_roundtrip.epg";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    auto app = std::make_unique<app::AppService>(std::make_unique<ZipEpgStorage>());

    app->newDocument(app::Size{8, 8}, 72.f, common::colors::Transparent);

    app::LayerSpec s{};
    s.color = 0x112233FFu; // layer 1
    app->addLayer(s);
    s.color = 0x445566FFu; // layer 2
    app->addLayer(s);

    ASSERT_EQ(app->document().layerCount(), 3u); // BG + 2 layers

    app->save(tmp.string());
    app->closeDocument();
    app->open(tmp.string());

    ASSERT_TRUE(app->hasDocument());
    EXPECT_EQ(app->document().layerCount(), 3u);

    std::filesystem::remove(tmp, ec);
}