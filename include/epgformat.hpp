#pragma once
#include <QImage>
#include <QString>

namespace EpgFormat
{
bool save(const QString& fileName, const QImage& image);
bool load(const QString& fileName, QImage& outImage);
}  // namespace EpgFormat
