#pragma once
#include <QString>
#include <QImage>

namespace EpgFormat
{
    bool save(const QString& fileName, const QImage& image);
    bool load(const QString& fileName, QImage& outImage);
}
