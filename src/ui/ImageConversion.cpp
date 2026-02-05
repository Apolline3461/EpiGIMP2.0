#include "ui/ImageConversion.hpp"

#include <QRgb>

#include <algorithm>

namespace ImageConversion
{

ImageBuffer qImageToImageBuffer(const QImage& image, int width, int height)
{
    ImageBuffer buf(width, height);

    // Process only the area that overlaps between the image and the buffer
    // Note: The buffer is already initialized to transparent by the constructor
    const int copyW = std::min(width, image.width());
    const int copyH = std::min(height, image.height());

    for (int y = 0; y < copyH; ++y)
    {
        for (int x = 0; x < copyW; ++x)
        {
            const QRgb p = image.pixel(x, y);
            const uint8_t r = qRed(p);
            const uint8_t g = qGreen(p);
            const uint8_t b = qBlue(p);
            const uint8_t a = qAlpha(p);
            const uint32_t rgba = (static_cast<uint32_t>(r) << 24) |
                                  (static_cast<uint32_t>(g) << 16) |
                                  (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
            buf.setPixel(x, y, rgba);
        }
    }

    return buf;
}

}  // namespace ImageConversion

QImage ImageConversion::imageBufferToQImage(const ImageBuffer& buf, QImage::Format fmt)
{
    QImage img(buf.width(), buf.height(), fmt);
    // Initialize to transparent just in case
    img.fill(Qt::transparent);

    for (int y = 0; y < buf.height(); ++y)
    {
        for (int x = 0; x < buf.width(); ++x)
        {
            const uint32_t rgba = buf.getPixel(x, y);
            const uint8_t r = static_cast<uint8_t>((rgba >> 24) & 0xFF);
            const uint8_t g = static_cast<uint8_t>((rgba >> 16) & 0xFF);
            const uint8_t b = static_cast<uint8_t>((rgba >> 8) & 0xFF);
            const uint8_t a = static_cast<uint8_t>(rgba & 0xFF);
            img.setPixel(x, y, qRgba(r, g, b, a));
        }
    }

    return img;
}
