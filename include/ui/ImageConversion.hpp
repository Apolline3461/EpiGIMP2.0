#pragma once

#include <QImage>

#include "core/ImageBuffer.hpp"

namespace ImageConversion
{
/**
 * @brief Converts a QImage to an ImageBuffer with the specified dimensions
 * @param image The source QImage to convert
 * @param width The width of the output ImageBuffer
 * @param height The height of the output ImageBuffer
 * @return An ImageBuffer containing the converted image data
 * 
 * This function converts a QImage to an ImageBuffer by iterating through pixels
 * and converting from Qt's ARGB format to our RGBA format.
 * The conversion only processes pixels within the bounds of both the source image
 * and the specified output dimensions.
 */
ImageBuffer qImageToImageBuffer(const QImage& image, int width, int height);
/**
 * @brief Converts an ImageBuffer to a QImage using the given format.
 * @param buf The source ImageBuffer
 * @param fmt The desired QImage format (default ARGB32)
 * @return A QImage containing the converted pixels
 */
QImage imageBufferToQImage(const ImageBuffer& buf, QImage::Format fmt = QImage::Format_ARGB32);
}  // namespace ImageConversion
