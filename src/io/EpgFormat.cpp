#include "io/EpgFormat.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
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
static const char MAGIC[] = "EPIGIMP";

static void png_write_callback(void* context, void* data, int size)
{
    auto* vec = static_cast<std::vector<uint8_t>*>(context);
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    vec->insert(vec->end(), bytes, bytes + size);
}

bool save(const std::string& fileName, const ImageBuffer& image)
{
    // Encode image as PNG into memory using stb
    const int width = image.width();
    const int height = image.height();
    const int comp = 4;  // RGBA
    std::vector<uint8_t> pngData;
    stbi_write_png_to_func(png_write_callback, &pngData, width, height, comp, image.data(),
                           width * comp);
    // Write custom container: MAGIC, version, width, height, comp, then PNG data size + data
    std::ofstream ofs(fileName, std::ios::binary);
    if (!ofs)
        return false;

    ofs.write(MAGIC, sizeof(MAGIC) - 1);

    const int32_t version = 1;
    ofs.write(reinterpret_cast<const char*>(&version), sizeof(version));

    int32_t w = width;
    int32_t h = height;
    int32_t c = comp;
    ofs.write(reinterpret_cast<const char*>(&w), sizeof(w));
    ofs.write(reinterpret_cast<const char*>(&h), sizeof(h));
    ofs.write(reinterpret_cast<const char*>(&c), sizeof(c));

    int32_t dataSize = static_cast<int32_t>(pngData.size());
    ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    if (dataSize > 0)
        ofs.write(reinterpret_cast<const char*>(pngData.data()), dataSize);

    return ofs.good();
}

bool load(const std::string& fileName, ImageBuffer& outImage)
{
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs)
        return false;

    char magicBuf[sizeof(MAGIC)];
    ifs.read(magicBuf, sizeof(MAGIC) - 1);
    if (ifs.gcount() != static_cast<std::streamsize>(sizeof(MAGIC) - 1))
        return false;
    if (std::string(magicBuf, sizeof(MAGIC) - 1) != std::string(MAGIC))
        return false;

    int32_t version = 0;
    ifs.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1)
        return false;

    int32_t w = 0, h = 0, c = 0;
    ifs.read(reinterpret_cast<char*>(&w), sizeof(w));
    ifs.read(reinterpret_cast<char*>(&h), sizeof(h));
    ifs.read(reinterpret_cast<char*>(&c), sizeof(c));

    int32_t dataSize = 0;
    ifs.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
    if (dataSize <= 0)
        return false;

    std::vector<uint8_t> pngData(static_cast<size_t>(dataSize));
    ifs.read(reinterpret_cast<char*>(pngData.data()), dataSize);
    if (ifs.gcount() != dataSize)
        return false;

    int imgW = 0, imgH = 0, imgComp = 0;
    unsigned char* decoded =
        stbi_load_from_memory(pngData.data(), dataSize, &imgW, &imgH, &imgComp, 4);
    if (!decoded)
        return false;

    // Create output ImageBuffer and copy pixels (stb gives RGBA)
    outImage = ImageBuffer(imgW, imgH);
    const size_t total = static_cast<size_t>(imgW) * static_cast<size_t>(imgH) * 4u;
    std::memcpy(outImage.data(), decoded, total);

    stbi_image_free(decoded);
    return true;
}

}  // namespace EpgFormat
