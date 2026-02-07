#include "ui/image.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QRubberBand>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>

#include "ui/window.hpp"

void ImageActions::newImage(MainWindow* window)
{
    QDialog dialog(window);
    dialog.setWindowTitle(QObject::tr("Nouvelle image"));

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    // Largeur
    QHBoxLayout* widthLayout = new QHBoxLayout();
    QLabel* widthLabel = new QLabel(QObject::tr("Largeur (px):"));
    QSpinBox* widthSpinBox = new QSpinBox();
    widthSpinBox->setRange(1, 10000);
    widthSpinBox->setValue(800);
    widthLayout->addWidget(widthLabel);
    widthLayout->addWidget(widthSpinBox);
    widthLayout->addStretch();

    // Hauteur
    QHBoxLayout* heightLayout = new QHBoxLayout();
    QLabel* heightLabel = new QLabel(QObject::tr("Hauteur (px):"));
    QSpinBox* heightSpinBox = new QSpinBox();
    heightSpinBox->setRange(1, 10000);
    heightSpinBox->setValue(600);
    heightLayout->addWidget(heightLabel);
    heightLayout->addWidget(heightSpinBox);
    heightLayout->addStretch();

    // Boutons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    mainLayout->addLayout(widthLayout);
    mainLayout->addLayout(heightLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted)
    {
        if (!window->myApp_)
            return;
        std::uint32_t bg = 0xFFFFFFFFu;

        int width = widthSpinBox->value();
        int height = heightSpinBox->value();

        window->myApp_->newDocument(app::Size{width, height}, 72.f, bg);

        window->m_currentFileName.clear();
        window->populateLayersList();
        window->updateImageDisplay();
        window->m_scrollArea->setVisible(true);

        window->statusBar()->showMessage(
            QObject::tr("Nouvelle image créée: %1x%2").arg(width).arg(height));
        window->setWindowTitle(QObject::tr("Sans titre - EpiGimp 2.0"));
    }
}

void ImageActions::openImage(MainWindow* window)
{
    if (!window->myApp_)
        return;
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString fileName = QFileDialog::getOpenFileName(
        window, QObject::tr("Ouvrir une image"), picturesPath,
        QObject::tr(
            "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.webp);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
        return;

    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage image = reader.read();

    if (image.isNull())
    {
        QMessageBox::critical(window, QObject::tr("Erreur"),
                              QObject::tr("Impossible de charger l'image %1:\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(reader.errorString()));
        return;
    }

    QImage img = image.convertToFormat(QImage::Format_ARGB32);

    std::vector<std::uint32_t> pixels;
    pixels.resize(static_cast<std::size_t>(img.width() * img.height()));
    for (int y = 0; y < img.height(); ++y)
    {
        for (int x = 0; x < img.width(); ++x)
        {
            const QRgb px = img.pixel(x, y);
            const std::uint32_t r = static_cast<std::uint32_t>(qRed(px));
            const std::uint32_t g = static_cast<std::uint32_t>(qGreen(px));
            const std::uint32_t b = static_cast<std::uint32_t>(qBlue(px));
            const std::uint32_t a = static_cast<std::uint32_t>(qAlpha(px));
            // ton format core: 0xRRGGBBAA
            pixels[static_cast<std::size_t>(y * img.width() + x)] =
                (r << 24) | (g << 16) | (b << 8) | a;
        }
    }
    window->myApp_->loadRasterAsNewDocument(app::Size{img.width(), img.height()}, 72.f, pixels);

    window->m_currentFileName = fileName;

    window->populateLayersList();
    window->updateImageDisplay();
    window->m_scrollArea->setVisible(true);

    QString message = QObject::tr("Image chargée: %1 (%2x%3)")
                          .arg(QFileInfo(fileName).fileName())
                          .arg(window->m_currentImage.width())
                          .arg(window->m_currentImage.height());
    window->statusBar()->showMessage(message);
    window->setWindowTitle(QObject::tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
}

void ImageActions::saveImage(MainWindow* window)
{
    if (window->m_currentImage.isNull())
    {
        QMessageBox::information(window, QObject::tr("Information"),
                                 QObject::tr("Aucune image à sauvegarder."));
        return;
    }

    QString picturesPath =
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/untitled.png";

    QFileDialog dialog(window, QObject::tr("Enregistrer l'image"), picturesPath);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilters({QObject::tr("PNG (*.png)"), QObject::tr("JPEG (*.jpg *.jpeg)"),
                           QObject::tr("Tous les fichiers (*)")});
    dialog.selectNameFilter(QObject::tr("PNG (*.png)"));
    if (!dialog.exec())
        return;
    QString fileName = dialog.selectedFiles().value(0);
    QString selectedFilter = dialog.selectedNameFilter();
    QString format;

    if (fileName.isEmpty())
        return;

    if (selectedFilter.contains("PNG", Qt::CaseInsensitive))
    {
        format = "png";
        if (!fileName.endsWith(".png", Qt::CaseInsensitive))
            fileName += ".png";
    }
    else if (selectedFilter.contains("JPEG", Qt::CaseInsensitive))
    {
        format = "jpg";
        if (!fileName.endsWith(".jpg", Qt::CaseInsensitive) &&
            !fileName.endsWith(".jpeg", Qt::CaseInsensitive))
            fileName += ".jpg";
    }
    else
    {
        // fallback: try to guess from extension, default to png
        if (fileName.endsWith(".jpg", Qt::CaseInsensitive) ||
            fileName.endsWith(".jpeg", Qt::CaseInsensitive))
        {
            format = "jpg";
        }
        else
        {
            format = "png";
            if (!fileName.endsWith(".png", Qt::CaseInsensitive))
                fileName += ".png";
        }
    }
    QImageWriter writer(fileName, format.toUtf8());
    if (!writer.write(window->m_currentImage))
    {
        QMessageBox::critical(window, QObject::tr("Erreur"),
                              QObject::tr("Impossible de sauvegarder l'image %1:\n%2")
                                  .arg(QDir::toNativeSeparators(fileName), writer.errorString()));
        return;
    }

    window->m_currentFileName = fileName;
    window->statusBar()->showMessage(QObject::tr("Image sauvegardée: %1").arg(fileName), 3000);
}

void ImageActions::closeImage(MainWindow* window)
{
    if (window->myApp_)
        window->myApp_->closeDocument();
    window->m_currentImage = QImage();
    window->m_currentFileName.clear();
    window->m_imageLabel->clear();
    window->m_scrollArea->setVisible(false);
    window->m_scaleFactor = 1.0;
    window->m_document.reset();
    window->populateLayersList();
    window->updateImageDisplay();

    window->setWindowTitle(QObject::tr("EpiGimp 2.0"));
    window->statusBar()->showMessage(QObject::tr("Image fermée"), 2000);
}

ImageLabel::ImageLabel(QWidget* parent)
    : QLabel(parent), m_rubberBand_(new QRubberBand(QRubberBand::Rectangle, this))
{
    setMouseTracking(true);
    m_hasSelection_ = false;
}

void ImageLabel::mousePressEvent(QMouseEvent* event)
{
    if (!m_selectionEnabled_)
    {
        QLabel::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        m_origin_ = event->pos();
        m_rubberBand_->setGeometry(QRect(m_origin_, QSize()));
        m_rubberBand_->show();
    }
    QLabel::mousePressEvent(event);
}

void ImageLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_selectionEnabled_)
    {
        QLabel::mouseMoveEvent(event);
        return;
    }

    if (m_rubberBand_->isVisible())
    {
        QRect rect(m_origin_, event->pos());
        m_rubberBand_->setGeometry(rect.normalized());
    }
    QLabel::mouseMoveEvent(event);
}

void ImageLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_selectionEnabled_)
    {
        QLabel::mouseReleaseEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton && m_rubberBand_->isVisible())
    {
        QRect rect(m_origin_, event->pos());
        rect = rect.normalized();
        // ensure non-empty
        if (rect.width() > 0 && rect.height() > 0)
        {
            m_selectionRect_ = rect;
            m_hasSelection_ = true;
            m_rubberBand_->hide();
            update();
            emit this->selectionFinished(rect);
        }
    }
    QLabel::mouseReleaseEvent(event);
}

void ImageLabel::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event);

    if (m_hasSelection_)
    {
        QPainter painter(this);
        QPen pen(Qt::red);
        pen.setStyle(Qt::DotLine);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(m_selectionRect_);
    }
}
