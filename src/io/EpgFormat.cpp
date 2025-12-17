#include "io/EpgFormat.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

extern "C"
{
#include <stb_image.h>
#include <stb_image_write.h>
}

namespace EpgFormat
{
static constexpr char MAGIC[] = "EPIGIMP";

static constexpr int32_t kVersion = 1;
static constexpr int32_t kChannels = 4;  // RGBA
struct EpgHeader
{
    int32_t version{};
    int32_t width{};
    int32_t height{};
    int32_t channels{};
    int32_t dataSize{};
};

// Signature imposée par stb_image_write
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void png_write_callback(void* context, void* data, int size)
{
    auto* vec = static_cast<std::vector<uint8_t>*>(context);
    const auto* bytes = static_cast<const uint8_t*>(data);
    if (!vec || !bytes || size <= 0)
        return;
    vec->insert(vec->end(), bytes, bytes + size);
}

static bool writeHeader(std::ofstream& ofs, const EpgHeader& hdr)
{
    if (!ofs)
        return false;

    ofs.write(reinterpret_cast<const char*>(&hdr.version), sizeof(hdr.version));
    ofs.write(reinterpret_cast<const char*>(&hdr.width), sizeof(hdr.width));
    ofs.write(reinterpret_cast<const char*>(&hdr.height), sizeof(hdr.height));
    ofs.write(reinterpret_cast<const char*>(&hdr.channels), sizeof(hdr.channels));
    ofs.write(reinterpret_cast<const char*>(&hdr.dataSize), sizeof(hdr.dataSize));
    return ofs.good();
}

static bool readHeader(std::ifstream& ifs, EpgHeader& hdr)
{
    if (!ifs)
        return false;

    char magicBuf[sizeof(MAGIC)]{};
    ifs.read(magicBuf, sizeof(MAGIC) - 1);
    if (ifs.gcount() != static_cast<std::streamsize>(sizeof(MAGIC) - 1))
        if (std::string(magicBuf, sizeof(MAGIC) - 1) != std::string(MAGIC))
            return false;
    ifs.read(reinterpret_cast<char*>(&hdr.version), sizeof(hdr.version));
    ifs.read(reinterpret_cast<char*>(&hdr.width), sizeof(hdr.width));
    ifs.read(reinterpret_cast<char*>(&hdr.height), sizeof(hdr.height));
    ifs.read(reinterpret_cast<char*>(&hdr.channels), sizeof(hdr.channels));
    ifs.read(reinterpret_cast<char*>(&hdr.dataSize), sizeof(hdr.dataSize));
    return !ifs.fail();
}

static bool encodePngToMemory(const ImageBuffer& image, std::vector<uint8_t>& out)
{
    const int width = image.width();
    const int height = image.height();
    const int comp = kChannels;  // RGBA
    out.clear();

    if (width <= 0 || height <= 0)
        return false;
    if (comp <= 0)
        return false;
    if (width > (std::numeric_limits<int>::max() / comp))
        return false;
    const int stride = width * comp;
    if (stride <= 0)
        return false;
    const size_t expected =
        static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(comp);
    if (image.data() == nullptr)
        return false;
    stbi_write_png_to_func(png_write_callback, &out, width, height, comp, image.data(), stride);
    return !out.empty();
}

static unsigned char* decodePngFromMemory(const std::vector<uint8_t>& data, int& imgW, int& imgH,
                                          int& imgComp)
{
    if (data.empty())
        return nullptr;
    return stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &imgW, &imgH, &imgComp,
                                 kChannels);
}

bool save(const std::string& fileName, const ImageBuffer& image)
{
    std::vector<uint8_t> pngData;
    if (!encodePngToMemory(image, pngData))
        return false;

    std::ofstream ofs(fileName, std::ios::binary);
    if (!ofs)
        return false;

    EpgHeader hdr{};
    hdr.version = kVersion;
    hdr.width = static_cast<int32_t>(image.width());
    hdr.height = static_cast<int32_t>(image.height());
    hdr.channels = 4;
    hdr.dataSize = static_cast<int32_t>(pngData.size());

    if (!writeHeader(ofs, hdr))
        return false;

    if (hdr.dataSize > 0)
    {
        ofs.write(reinterpret_cast<const char*>(pngData.data()), hdr.dataSize);
        if (ofs.fail())
            return false;
    }

    return ofs.good();
}

bool load(const std::string& fileName, ImageBuffer& outImage)
{
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs)
        return false;

    int32_t version = 0;
    int32_t w = 0, h = 0, c = 0, dataSize = 0;

    EpgHeader hdr{};
    if (!readHeader(ifs, hdr))
        return false;

    if (hdr.version != kVersion)
        return false;
    if (hdr.width <= 0 || hdr.height <= 0 || hdr.width > MAX_DIM || hdr.height > MAX_DIM)
        return false;
    if (hdr.channels != kChannels)
        return false;
    if (int64_t pixelCount = static_cast<int64_t>(hdr.width) * static_cast<int64_t>(hdr.height);
        pixelCount <= 0 || pixelCount > MAX_PIXELS)
        return false;
    if (hdr.dataSize <= 0 || hdr.dataSize > (MAX_PIXELS * kChannels))
        return false;

    std::vector<uint8_t> pngData(static_cast<size_t>(hdr.dataSize));
    ifs.read(reinterpret_cast<char*>(pngData.data()), hdr.dataSize);
    if (ifs.fail() || ifs.gcount() != static_cast<std::streamsize>(hdr.dataSize))
        return false;

    int imgW = 0, imgH = 0, imgComp = 0;
    unsigned char* decoded = decodePngFromMemory(pngData, imgW, imgH, imgComp);
    if (!decoded)
        return false;

    // Create output ImageBuffer and copy pixels (stb gives RGBA)
    if (imgW <= 0 || imgH <= 0)
    {
        stbi_image_free(decoded);
        return false;
    }

    outImage = ImageBuffer(imgW, imgH);
    const size_t total =
        static_cast<size_t>(imgW) * static_cast<size_t>(imgH) * static_cast<size_t>(kChannels);
    std::memcpy(outImage.data(), decoded, total);

    stbi_image_free(decoded);
    return true;
}

}  // namespace EpgFormat
