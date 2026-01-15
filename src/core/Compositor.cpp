//
// Created by apolline on 09/01/2026.
//

#include "core/Compositor.hpp"

#include <algorithm>

#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"

static inline std::uint8_t extractR(std::uint32_t rgba)
{
    return static_cast<std::uint8_t>((rgba >> 24) & 0xFFu);
}

static inline std::uint8_t extractG(std::uint32_t rgba)
{
    return static_cast<std::uint8_t>((rgba >> 16) & 0xFFu);
}

static inline std::uint8_t extractB(std::uint32_t rgba)
{
    return static_cast<std::uint8_t>((rgba >> 8) & 0xFFu);
}

static inline std::uint8_t extractA(std::uint32_t rgba)
{
    return static_cast<std::uint8_t>(rgba & 0xFFu);
}

static inline std::uint32_t makePixel(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                      std::uint8_t a)
{
    return (static_cast<std::uint32_t>(r) << 24) | (static_cast<std::uint32_t>(g) << 16) |
           (static_cast<std::uint32_t>(b) << 8) | static_cast<std::uint32_t>(a);
}

static std::uint32_t blendPixel(std::uint32_t src, std::uint32_t dst, float layerOpacity)
{
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

    const auto toByte = [](float v) -> std::uint8_t
    {
        float clamped = std::clamp(v, 0.0f, 1.0f);
        return static_cast<std::uint8_t>(clamped * 255.0f + 0.5f);
    };

    return makePixel(toByte(outR), toByte(outG), toByte(outB), toByte(outA));
}

// ---- Fonction interne : compose une région du doc vers out -----------------
//
// docX0, docY0 : point haut/gauche dans le document
// roiW, roiH   : taille de la région
// out          : image de sortie (roiW x roiH)
// On assume que roi est dans les bornes du document (sinon on peut clamp).

static void composeRegion(const Document& doc, int docX0, int docY0, int roiW, int roiH,
                          ImageBuffer& out)
{
    if (roiW <= 0 || roiH <= 0)
        return;
    const int docW = doc.width();
    const int docH = doc.height();
    if (docW <= 0 || docH <= 0)
        return;
    if (docX0 >= docW || docY0 >= docH)
        return;
    const int maxW = std::min(roiW, docW - docX0);
    const int maxH = std::min(roiH, docH - docY0);
    if (maxW <= 0 || maxH <= 0)
        return;

    const int layerCount = doc.layerCount();
    if (layerCount == 0)
        return;

    for (int i = 0; i < layerCount; ++i)
    {
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

        for (int sy = 0; sy < maxH; ++sy)
        {
            const int docY = docY0 + sy;
            for (int sx = 0; sx < maxW; ++sx)
            {
                const int docX = docX0 + sx;
                const std::uint32_t src = imgPtr->getPixel(docX, docY);
                const std::uint32_t dst = out.getPixel(sx, sy);
                const std::uint32_t blended = blendPixel(src, dst, opacity);
                out.setPixel(sx, sy, blended);
            }
        }
    }
}

void Compositor::compose(const Document& doc, ImageBuffer& out) const
{
    const int width = doc.width();
    const int height = doc.height();

    if (out.width() != width || out.height() != height)
        return;
    out.fill(0u);
    composeRegion(doc, 0, 0, width, height, out);
}

void Compositor::composeROI(const Document& doc, int x, int y, int w, int h, ImageBuffer& out) const
{
    if (w <= 0 || h <= 0)
        return;
    if (out.width() != w || out.height() != h)
        return;

    out.fill(0u);
    composeRegion(doc, x, y, w, h, out);
}