//
// Created by apolline on 12/03/2026.
//

#include "core/Transform.hpp"

#include <algorithm>
#include <cmath>

namespace core
{
static inline double degToRad(double deg)
{
    return deg * M_PI / 180.0;
}

static inline float normDeg(float deg)
{
    // normalise dans [0, 360)
    float a = std::fmod(deg, 360.0f);
    if (a < 0.0f)
        a += 360.0f;
    return a;
}

static inline bool isClose(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

std::shared_ptr<ImageBuffer> rotate(const ImageBuffer& src, float degrees, std::uint32_t bgColor)
{
    const int w = src.width();
    const int h = src.height();

    const double rad = degToRad(static_cast<double>(degrees));
    const double c = std::cos(rad);
    const double s = std::sin(rad);

    // bounding box size
    const double aw = std::abs(c) * w + std::abs(s) * h;
    const double ah = std::abs(s) * w + std::abs(c) * h;

    const int newW = std::max(1, static_cast<int>(std::ceil(aw)));
    const int newH = std::max(1, static_cast<int>(std::ceil(ah)));

    auto out = std::make_shared<ImageBuffer>(newW, newH);

    const double srcCx = (w - 1) * 0.5;
    const double srcCy = (h - 1) * 0.5;
    const double dstCx = (newW - 1) * 0.5;
    const double dstCy = (newH - 1) * 0.5;

    for (int y = 0; y < newH; ++y)
    {
        for (int x = 0; x < newW; ++x)
        {
            const double dx = static_cast<double>(x) - dstCx;
            const double dy = static_cast<double>(y) - dstCy;

            // inverse mapping (R(-a))
            const double sx = c * dx + s * dy + srcCx;
            const double sy = -s * dx + c * dy + srcCy;

            const int ix = static_cast<int>(std::lround(sx));
            const int iy = static_cast<int>(std::lround(sy));

            if (ix >= 0 && ix < w && iy >= 0 && iy < h)
                out->setPixel(x, y, src.getPixel(ix, iy));
            else
                out->setPixel(x, y, bgColor);
        }
    }

    return out;
}

}  // namespace core