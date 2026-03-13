// Created by apolline on 12/03/2026.

#include "core/Transform.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

namespace core
{
static inline double degToRad(double deg)
{
    return deg * M_PI / 180.0;
}

static inline float normDeg(float deg)
{
    float a = std::fmod(deg, 360.0f);
    if (a < 0.0f)
        a += 360.0f;
    return a;
}

static inline bool isClose(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

static inline void fillImage(ImageBuffer& img, std::uint32_t c)
{
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            img.setPixel(x, y, c);
}

// Convention: degrees > 0 == rotation horaire (CW)
std::shared_ptr<ImageBuffer> rotate(const ImageBuffer& src, float degrees, std::uint32_t bgColor)
{
    const int w = src.width();
    const int h = src.height();

    if (w <= 0 || h <= 0)
        return std::make_shared<ImageBuffer>(std::max(1, w), std::max(1, h));

    const float a = normDeg(degrees);

    // 0 / 360 : identité stricte
    if (isClose(a, 0.f) || isClose(a, 360.f))
        return std::make_shared<ImageBuffer>(src);

    // Rotations exactes (pas d'arrondi foireux, pas de trous)
    if (isClose(a, 90.f))
    {
        auto out = std::make_shared<ImageBuffer>(h, w);
        // CW: (x,y) -> (h-1-y, x)
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                out->setPixel(h - 1 - y, x, src.getPixel(x, y));
        return out;
    }

    if (isClose(a, 180.f))
    {
        auto out = std::make_shared<ImageBuffer>(w, h);
        // (x,y) -> (w-1-x, h-1-y)
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                out->setPixel(w - 1 - x, h - 1 - y, src.getPixel(x, y));
        return out;
    }

    if (isClose(a, 270.f))
    {
        auto out = std::make_shared<ImageBuffer>(h, w);
        // CW 270: (x,y) -> (y, w-1-x)
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                out->setPixel(y, w - 1 - x, src.getPixel(x, y));
        return out;
    }

    // --- Général (ex: 45°) : no-crop + pas de perte (forward mapping + splat) ---
    const double rad = degToRad(static_cast<double>(a));
    const double c = std::cos(rad);
    const double s = std::sin(rad);

    // bounding box size (rotation around center)
    const double aw = std::abs(c) * w + std::abs(s) * h;
    const double ah = std::abs(s) * w + std::abs(c) * h;

    const int newW = std::max(1, static_cast<int>(std::ceil(aw)));
    const int newH = std::max(1, static_cast<int>(std::ceil(ah)));

    auto out = std::make_shared<ImageBuffer>(newW, newH);
    fillImage(*out, bgColor);

    // centres pixel
    const double srcCx = (w - 1) * 0.5;
    const double srcCy = (h - 1) * 0.5;
    const double dstCx = (newW - 1) * 0.5;
    const double dstCy = (newH - 1) * 0.5;

    auto put = [&](int x, int y, std::uint32_t px)
    {
        if (x >= 0 && x < newW && y >= 0 && y < newH)
            out->setPixel(x, y, px);
    };

    // Forward mapping (src -> dst) avec splat 2x2 pour éviter la disparition de pixels
    for (int sy = 0; sy < h; ++sy)
    {
        for (int sx = 0; sx < w; ++sx)
        {
            const std::uint32_t px = src.getPixel(sx, sy);

            if (px == bgColor)
                continue;

            const double dx = static_cast<double>(sx) - srcCx;
            const double dy = static_cast<double>(sy) - srcCy;

            // CW rotation:
            // [ x' ] = [  c  -s ] [dx] + center
            // [ y' ]   [  s   c ] [dy]
            const double fx = c * dx - s * dy + dstCx;
            const double fy = s * dx + c * dy + dstCy;

            const int x0 = static_cast<int>(std::floor(fx));
            const int y0 = static_cast<int>(std::floor(fy));
            const int x1 = x0 + 1;
            const int y1 = y0 + 1;

            // splat 2x2
            put(x0, y0, px);
            put(x1, y0, px);
            put(x0, y1, px);
            put(x1, y1, px);
        }
    }

    return out;
}

}  // namespace core