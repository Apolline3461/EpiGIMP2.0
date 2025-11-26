#include "../../include/io/epgformat.hpp"

#include <QBuffer>
#include <QDataStream>
#include <QFile>

namespace EpgFormat
{
bool save(const QString& fileName, const QImage& image)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);

    out << QString("EPIGIMP");
    out << qint32(1);
    out << image.width();
    out << image.height();
    out << qint32(image.format());

    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    out << imageData;
    return true;
}

bool load(const QString& fileName, QImage& outImage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    QString header;
    in >> header;
    if (header != "EPIGIMP")
        return false;

    qint32 version;
    in >> version;
    if (version != 1)
        return false;

    int width, height;
    qint32 format;
    in >> width >> height >> format;

    QByteArray imageData;
    in >> imageData;

    QBuffer buffer(&imageData);
    buffer.open(QIODevice::ReadOnly);
    outImage.load(&buffer, "PNG");

    return !outImage.isNull();
}
}  // namespace EpgFormat
