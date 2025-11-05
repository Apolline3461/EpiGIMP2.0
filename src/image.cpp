#include "image.hpp"

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
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>

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
        int width = widthSpinBox->value();
        int height = heightSpinBox->value();

        window->m_currentImage = QImage(width, height, QImage::Format_ARGB32);
        window->m_currentImage.fill(Qt::white);
        window->m_currentFileName.clear();

        window->updateImageDisplay();
        window->m_scrollArea->setVisible(true);

        QString message = QObject::tr("Nouvelle image créée: %1x%2").arg(width).arg(height);
        window->statusBar()->showMessage(message);
        window->setWindowTitle(QObject::tr("Sans titre - EpiGimp 2.0"));
    }
}

void ImageActions::openImage(MainWindow* window)
{
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString fileName = QFileDialog::getOpenFileName(
        window, QObject::tr("Ouvrir une image"), picturesPath,
        QObject::tr(
            "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.webp);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
        return;

    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();

    if (newImage.isNull())
    {
        QMessageBox::critical(window, QObject::tr("Erreur"),
                              QObject::tr("Impossible de charger l'image %1:\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(reader.errorString()));
        return;
    }

    window->m_currentImage = newImage;
    window->m_currentFileName = fileName;

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

    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString fileName = QFileDialog::getSaveFileName(
        window, QObject::tr("Enregistrer l'image"), picturesPath,
        QObject::tr("PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
        return;

    QImageWriter writer(fileName);
    if (!writer.write(window->m_currentImage))
    {
        QMessageBox::critical(window, QObject::tr("Erreur"),
                              QObject::tr("Impossible de sauvegarder l'image %1:\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(writer.errorString()));
        return;
    }

    window->m_currentFileName = fileName;
    window->statusBar()->showMessage(QObject::tr("Image sauvegardée: %1").arg(fileName), 3000);
}

void ImageActions::closeImage(MainWindow* window)
{
    window->m_currentImage = QImage();
    window->m_currentFileName.clear();
    window->m_imageLabel->clear();
    window->m_scrollArea->setVisible(false);
    window->m_scaleFactor = 1.0;

    window->setWindowTitle(QObject::tr("EpiGimp 2.0"));
    window->statusBar()->showMessage(QObject::tr("Image fermée"), 2000);
}
