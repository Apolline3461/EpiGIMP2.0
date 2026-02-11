//
// Created by apolline on 25/11/2025.
//

#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

#include <gtest/gtest.h>

using std::make_shared;
using std::string;

TEST(LayerTest, DefaultState)
{
    constexpr uint64_t id = 42;
    const string name = "test";
    auto buffer = make_shared<ImageBuffer>(3, 2);

    const Layer layerTested{id, name, buffer};

    EXPECT_EQ(layerTested.id(), id);
    EXPECT_EQ(layerTested.name(), name);
    EXPECT_TRUE(layerTested.visible());
    EXPECT_FALSE(layerTested.locked());
    EXPECT_FLOAT_EQ(layerTested.opacity(), 1.0f);

    EXPECT_NE(layerTested.image(), nullptr);
    EXPECT_EQ(layerTested.image(), buffer);
}

TEST(LayerTest, GivenValue)
{
    constexpr uint64_t id = 42;
    const string name = "testL";
    auto buffer = make_shared<ImageBuffer>(3, 2);
    constexpr bool visible = false;
    constexpr bool locked = true;
    constexpr float opacity = 0.0f;

    const Layer layerTested{id, name, buffer, visible, locked, opacity};

    EXPECT_EQ(layerTested.id(), id);
    EXPECT_EQ(layerTested.name(), name);
    EXPECT_FALSE(layerTested.visible());
    EXPECT_TRUE(layerTested.locked());
    EXPECT_FLOAT_EQ(layerTested.opacity(), opacity);

    EXPECT_NE(layerTested.image(), nullptr);
    EXPECT_EQ(layerTested.image(), buffer);
}

TEST(LayerTest, RenameChangeOnlyName)
{
    constexpr uint64_t id = 42;
    const string name = "testName";
    auto buffer = make_shared<ImageBuffer>(3, 2);

    Layer layerTested{id, name, buffer};

    layerTested.setName("NewName");

    EXPECT_EQ(layerTested.name(), "NewName");
    EXPECT_EQ(layerTested.id(), id);
}

TEST(LayerTest, VisibilityFlagIsMutable)
{
    constexpr uint64_t id = 42;
    const string name = "testName";
    auto buffer = make_shared<ImageBuffer>(3, 2);

    Layer layerTested{id, name, buffer, false};

    EXPECT_FALSE(layerTested.visible());
    layerTested.setVisible(true);
    EXPECT_TRUE(layerTested.visible());
}

TEST(LayerTest, LockFlagAndIsEditable)
{
    constexpr std::uint64_t id = 1;
    const string name = "lock-test";
    auto buffer = make_shared<ImageBuffer>(2, 2);

    Layer layer{id, name, buffer};

    // Case 1 : visible = true, locked = false → isEditable() = true
    EXPECT_TRUE(layer.visible());
    EXPECT_FALSE(layer.locked());
    EXPECT_TRUE(layer.isEditable());

    // Case 2 : locked = true → isEditable() == false
    layer.setLocked(true);
    EXPECT_TRUE(layer.locked());
    EXPECT_FALSE(layer.isEditable());

    // Case 3 : visible = false, locked = true →  isEditable() = false
    layer.setVisible(false);
    EXPECT_FALSE(layer.visible());
    EXPECT_TRUE(layer.locked());
    EXPECT_FALSE(layer.isEditable());
}

TEST(LayerTest, OpacityValuesAreClampedBetweenZeroAndOne)
{
    constexpr std::uint64_t id = 2;
    const string name = "opacity-test";
    auto buffer = make_shared<ImageBuffer>(2, 2);

    Layer layer{id, name, buffer};

    layer.setOpacity(0.0f);
    EXPECT_FLOAT_EQ(layer.opacity(), 0.0f);

    layer.setOpacity(0.5f);
    EXPECT_FLOAT_EQ(layer.opacity(), 0.5f);

    layer.setOpacity(1.0f);
    EXPECT_FLOAT_EQ(layer.opacity(), 1.0f);

    // Error case handle
    layer.setOpacity(-0.5f);
    EXPECT_FLOAT_EQ(layer.opacity(), 0.0f);

    layer.setOpacity(2.0f);
    EXPECT_FLOAT_EQ(layer.opacity(), 1.0f);
}

TEST(LayerTest, ImageSharedPtrIsKeptAndCanBeShared)
{
    constexpr std::uint64_t id1 = 10;
    constexpr std::uint64_t id2 = 11;
    const string name1 = "L1";
    const string name2 = "L2";

    auto image = make_shared<ImageBuffer>(3, 2);

    Layer l1{id1, name1, image};
    EXPECT_NE(l1.image(), nullptr);
    EXPECT_EQ(l1.image(), image);

    Layer l2{id2, name2, image};
    EXPECT_EQ(l2.image(), image);
    EXPECT_EQ(l1.image(), l2.image());

    auto otherImage = make_shared<ImageBuffer>(4, 4);
    l1.setImageBuffer(otherImage);

    EXPECT_EQ(l1.image(), otherImage);
    EXPECT_EQ(l2.image(), image);
}

TEST(LayerTest, OffsetDefaultAndMutable)
{
    constexpr std::uint64_t id = 77;
    const string name = "offset-test";
    auto buffer = make_shared<ImageBuffer>(5, 5);

    Layer layer{id, name, buffer};

    EXPECT_EQ(layer.offsetX(), 0);
    EXPECT_EQ(layer.offsetY(), 0);

    layer.setOffset(10, -3);
    EXPECT_EQ(layer.offsetX(), 10);
    EXPECT_EQ(layer.offsetY(), -3);
}

TEST(LayerTest, SetImageBufferToNull)
{
    constexpr std::uint64_t id = 80;
    const string name = "null-image-test";
    auto buffer = make_shared<ImageBuffer>(2, 2);

    Layer layer{id, name, buffer};
    EXPECT_NE(layer.image(), nullptr);

    layer.setImageBuffer(nullptr);
    EXPECT_EQ(layer.image(), nullptr);
}

TEST(LayerTest, EmptyNameIsAllowed)
{
    constexpr std::uint64_t id = 81;
    const string name = "non-empty";
    auto buffer = make_shared<ImageBuffer>(1, 1);

    Layer layer{id, name, buffer};
    layer.setName("");

    EXPECT_EQ(layer.name(), "");
    EXPECT_EQ(layer.id(), id);
}

TEST(LayerTest, CopyingCreatesIndependentNamesButSharedImageAndSameId)
{
    constexpr std::uint64_t id = 90;
    const string name = "original";
    auto buffer = make_shared<ImageBuffer>(3, 3);

    Layer l1{id, name, buffer};
    Layer l2 = l1;  // copy

    // id is copied
    EXPECT_EQ(l2.id(), l1.id());

    // image shared_ptr points to the same buffer
    EXPECT_EQ(l2.image(), l1.image());

    // changing the name of the copy does not change the original
    l2.setName("copied");
    EXPECT_EQ(l2.name(), "copied");
    EXPECT_EQ(l1.name(), name);
}