//
// Created by apolline on 09/01/2026.
//

#include "core/Compositor.hpp"

#include <algorithm>

#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

static inline std::uint8_t extractR(std::uint32_t rgba) {
    return static_cast<std::uint8_t>((rgba >> 24) & 0xFFu);
}

static inline std::uint8_t extractG(std::uint32_t rgba) {
    return static_cast<std::uint8_t>((rgba >> 16) & 0xFFu);
}

static inline std::uint8_t extractB(std::uint32_t rgba) {
    return static_cast<std::uint8_t>((rgba >> 8) & 0xFFu);
}

static inline std::uint8_t extractA(std::uint32_t rgba) {
    return static_cast<std::uint8_t>(rgba & 0xFFu);
}

static inline std::uint32_t makePixel(std::uint8_t r,
                                      std::uint8_t g,
                                      std::uint8_t b,
                                      std::uint8_t a) {
    return (static_cast<std::uint32_t>(r) << 24) |
           (static_cast<std::uint32_t>(g) << 16) |
           (static_cast<std::uint32_t>(b) << 8)  |
           static_cast<std::uint32_t>(a);
}

static std::uint32_t blendPixel(std::uint32_t src,
                                std::uint32_t dst,
                                float layerOpacity) {
    const float srcR = extractR(src) / 255.0f;
    const float srcG = extractG(src) / 255.0f;
    const float srcB = extractB(src) / 255.0f;
    const float srcA = extractA(src) / 255.0f;

    const float dstR = extractR(dst) / 255.0f;
    const float dstG = extractG(dst) / 255.0f;
    const float dstB = extractB(dst) / 255.0f;
    const float dstA = extractA(dst) / 255.0f;

    // On applique l'opacité du layer sur l'alpha source
    const float effA = srcA * std::clamp(layerOpacity, 0.0f, 1.0f);

    // Alpha out (formule "src over dst")
    const float outA = effA + dstA * (1.0f - effA);

    if (outA <= 0.0f)
        return 0u;

    // RGB out, en "straight alpha"
    const float outR = (srcR * effA + dstR * dstA * (1.0f - effA)) / outA;
    const float outG = (srcG * effA + dstG * dstA * (1.0f - effA)) / outA;
    const float outB = (srcB * effA + dstB * dstA * (1.0f - effA)) / outA;

    const auto toByte = [](float v) -> std::uint8_t {
        float clamped = std::clamp(v, 0.0f, 1.0f);
        return static_cast<std::uint8_t>(clamped * 255.0f + 0.5f);
    };

    return makePixel(
        toByte(outR),
        toByte(outG),
        toByte(outB),
        toByte(outA)
    );
}

void Compositor::compose(const Document& doc, ImageBuffer& out) const
{

    const int width  = doc.width();
    const int height = doc.height();

    out.fill(0u);

    const int count = doc.layerCount();
    if (count == 0) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        const auto layer = doc.layerAt(i);
        if (!layer)
            continue;

        if (!layer->visible())
            continue;

        const auto& imgPtr = layer->image();
        if (!imgPtr)
            continue;

        float opacity = layer->opacity();
        if (opacity <= 0.0f)
            continue;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::uint32_t src = imgPtr->getPixel(x, y);
                const std::uint32_t dst = out.getPixel(x, y);
                const std::uint32_t blended = blendPixel(src, dst, opacity);
                out.setPixel(x, y, blended);
            }
        }
    }
}

