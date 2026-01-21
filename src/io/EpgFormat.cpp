#include "io/EpgFormat.hpp"

#include <stb_image.h>

#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "io/EpgJson.hpp"

// Callback used by stb_image_write to collect the encoded PNG bytes.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void pngWriteCallback(void* context, void* data, int size)
{
    auto* buffer = reinterpret_cast<std::vector<unsigned char>*>(context);
    const auto* bytes = static_cast<const unsigned char*>(data);
    buffer->insert(buffer->end(), bytes, bytes + size);
}

std::string getCurrentTimestampUTC()
{
    auto now = std::chrono::system_clock::now();
    std::time_t const t = std::chrono::system_clock::to_time_t(now);
    std::tm const tm = *std::gmtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string formatLayerId(size_t index)
{
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << (index + 1);
    return oss.str();
}

std::unique_ptr<ImageBuffer> decodePngToImageBuffer(const std::vector<unsigned char>& pngData)
{
    // Quick sanity: check PNG signature (8 bytes)
    if (pngData.size() < 8 || memcmp(pngData.data(), kPngSignature, 8) != 0)
        throw std::runtime_error("Not a PNG file (too small)");

    int width = 0, height = 0, channels = 0;
    unsigned char* decoded = stbi_load_from_memory(pngData.data(), static_cast<int>(pngData.size()),
                                                   &width, &height, &channels, 4);

    if (!decoded)
        throw std::runtime_error(
            std::string("stb_image: impossible de décoder le PNG en mémoire: ") +
            stbi_failure_reason());

    auto buf = std::make_unique<ImageBuffer>(width, height);
    size_t const bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    std::memcpy(buf->data(), decoded, bytes);

    stbi_image_free(decoded);
    return buf;
}
