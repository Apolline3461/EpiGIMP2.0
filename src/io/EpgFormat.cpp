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
static constexpr int32_t MAX_DATA_SIZE = 100 * 1024 * 1024;  // 100 MB limit to prevent DoS

static void png_write_callback(void* context, void* data, int size)
{
    auto* vec = static_cast<std::vector<uint8_t>*>(context);
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    vec->insert(vec->end(), bytes, bytes + size);
}

static bool writeHeader(std::ofstream& ofs, int32_t w, int32_t h, int32_t c, int32_t dataSize)
{
    if (!ofs)
        return false;

    ofs.write(MAGIC, sizeof(MAGIC) - 1);
    const int32_t version = 1;
    ofs.write(reinterpret_cast<const char*>(&version), sizeof(version));

    ofs.write(reinterpret_cast<const char*>(&w), sizeof(w));
    ofs.write(reinterpret_cast<const char*>(&h), sizeof(h));
    ofs.write(reinterpret_cast<const char*>(&c), sizeof(c));

    ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    return ofs.good();
}

static bool readHeader(std::ifstream& ifs, int32_t& version, int32_t& w, int32_t& h, int32_t& c,
                       int32_t& dataSize)
{
    if (!ifs)
        return false;

    char magicBuf[sizeof(MAGIC)];
    ifs.read(magicBuf, sizeof(MAGIC) - 1);
    if (ifs.gcount() != static_cast<std::streamsize>(sizeof(MAGIC) - 1))
        return false;
    if (std::string(magicBuf, sizeof(MAGIC) - 1) != std::string(MAGIC))
        return false;

    ifs.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (ifs.fail())
        return false;

    ifs.read(reinterpret_cast<char*>(&w), sizeof(w));
    if (ifs.fail())
        return false;
    ifs.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (ifs.fail())
        return false;
    ifs.read(reinterpret_cast<char*>(&c), sizeof(c));
    if (ifs.fail())
        return false;

    ifs.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
    if (ifs.fail())
        return false;

    return true;
}

static bool encodePngToMemory(const ImageBuffer& image, std::vector<uint8_t>& out)
{
    const int width = image.width();
    const int height = image.height();
    const int comp = 4;  // RGBA
    out.clear();
    stbi_write_png_to_func(png_write_callback, &out, width, height, comp, image.data(),
                           width * comp);
    return !out.empty();
}

static unsigned char* decodePngFromMemory(const std::vector<uint8_t>& data, int& imgW, int& imgH,
                                          int& imgComp)
{
    if (data.empty())
        return nullptr;
    return stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &imgW, &imgH, &imgComp,
                                 4);
}

bool save(const std::string& fileName, const ImageBuffer& image)
{
    std::vector<uint8_t> pngData;
    if (!encodePngToMemory(image, pngData))
        return false;

    std::ofstream ofs(fileName, std::ios::binary);
    if (!ofs)
        return false;

    const int32_t w = static_cast<int32_t>(image.width());
    const int32_t h = static_cast<int32_t>(image.height());
    const int32_t c = 4;
    const int32_t dataSize = static_cast<int32_t>(pngData.size());

    if (!writeHeader(ofs, w, h, c, dataSize))
        return false;

    if (dataSize > 0)
    {
        ofs.write(reinterpret_cast<const char*>(pngData.data()), dataSize);
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
    if (!readHeader(ifs, version, w, h, c, dataSize))
        return false;

    if (version != 1)
        return false;
    if (dataSize <= 0 || dataSize > MAX_DATA_SIZE)
        return false;

    std::vector<uint8_t> pngData(static_cast<size_t>(dataSize));
    ifs.read(reinterpret_cast<char*>(pngData.data()), dataSize);
    if (ifs.gcount() != dataSize || ifs.fail())
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
    const size_t total = static_cast<size_t>(imgW) * static_cast<size_t>(imgH) * 4u;
    std::memcpy(outImage.data(), decoded, total);

    stbi_image_free(decoded);
    return true;
}

}  // namespace EpgFormat
