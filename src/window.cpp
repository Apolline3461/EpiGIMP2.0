#include "window.hpp"

#include <QDataStream>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_imageLabel(new QLabel),
      m_scrollArea(new QScrollArea),
      m_scaleFactor(1.0),
      m_fileMenu(nullptr),
      m_viewMenu(nullptr)
{
    // Configuration du label d'image
    m_imageLabel->setBackgroundRole(QPalette::Base);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_imageLabel->setScaledContents(true);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    // Configuration de la zone de scroll
    m_scrollArea->setBackgroundRole(QPalette::Dark);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setVisible(false);
    m_scrollArea->setWidgetResizable(false);

    setCentralWidget(m_scrollArea);

    // Création de l'interface
    createActions();
    createMenus();

    // Barre de status
    statusBar()->showMessage(tr("Prêt"));

    // Configuration de la fenêtre
    setWindowTitle(tr("EpiGimp 2.0"));
    resize(1024, 768);
}

MainWindow::~MainWindow()
{
    // Les QObjects sont automatiquement détruits par Qt
}

void MainWindow::newImage()
{
    // Créer une boîte de dialogue personnalisée
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Nouvelle image"));

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    // Widget pour la largeur
    QHBoxLayout* widthLayout = new QHBoxLayout();
    QLabel* widthLabel = new QLabel(tr("Largeur (px):"));
    QSpinBox* widthSpinBox = new QSpinBox();
    widthSpinBox->setRange(1, 10000);
    widthSpinBox->setValue(800);
    widthSpinBox->setMinimumWidth(100);
    widthLayout->addWidget(widthLabel);
    widthLayout->addWidget(widthSpinBox);
    widthLayout->addStretch();

    // Widget pour la hauteur
    QHBoxLayout* heightLayout = new QHBoxLayout();
    QLabel* heightLabel = new QLabel(tr("Hauteur (px):"));
    QSpinBox* heightSpinBox = new QSpinBox();
    heightSpinBox->setRange(1, 10000);
    heightSpinBox->setValue(600);
    heightSpinBox->setMinimumWidth(100);
    heightLayout->addWidget(heightLabel);
    heightLayout->addWidget(heightSpinBox);
    heightLayout->addStretch();

    // Boutons OK/Annuler
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Ajouter tous les widgets au layout principal
    mainLayout->addLayout(widthLayout);
    mainLayout->addLayout(heightLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonBox);

    // Afficher la boîte de dialogue
    if (dialog.exec() == QDialog::Accepted)
    {
        int width = widthSpinBox->value();
        int height = heightSpinBox->value();

        // Créer une image blanche
        m_currentImage = QImage(width, height, QImage::Format_ARGB32);
        m_currentImage.fill(Qt::white);
        m_currentFileName.clear();

        updateImageDisplay();
        m_scrollArea->setVisible(true);

        const QString message = tr("Nouvelle image créée: %1x%2").arg(width).arg(height);

        statusBar()->showMessage(message);
        setWindowTitle(tr("Sans titre - EpiGimp 2.0"));
    }
}

void MainWindow::openImage()
{
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Ouvrir une image"), picturesPath,
        tr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.webp);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
    {
        return;
    }

    QImageReader reader(fileName);
    reader.setAutoTransform(true);

    const QImage newImage = reader.read();
    if (newImage.isNull())
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de charger l'image %1:\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(reader.errorString()));
        return;
    }

    m_currentImage = newImage;
    m_currentFileName = fileName;

    updateImageDisplay();
    m_scrollArea->setVisible(true);

    const QString message = tr("Image chargée: %1 (%2x%3)")
                                .arg(QFileInfo(fileName).fileName())
                                .arg(m_currentImage.width())
                                .arg(m_currentImage.height());

    statusBar()->showMessage(message);
    setWindowTitle(tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
}

void MainWindow::saveImage()
{
    if (m_currentImage.isNull())
    {
        QMessageBox::information(this, tr("Information"), tr("Aucune image à sauvegarder."));
        return;
    }

    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Enregistrer l'image"), picturesPath,
        tr("PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
    {
        return;
    }

    QImageWriter writer(fileName);
    if (!writer.write(m_currentImage))
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de sauvegarder l'image %1:\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(writer.errorString()));
        return;
    }

    m_currentFileName = fileName;
    statusBar()->showMessage(tr("Image sauvegardée: %1").arg(fileName), 3000);
}

void MainWindow::closeImage()
{
    m_currentImage = QImage();
    m_currentFileName.clear();
    m_imageLabel->clear();
    m_scrollArea->setVisible(false);
    m_scaleFactor = 1.0;

    setWindowTitle(tr("EpiGimp 2.0"));
    statusBar()->showMessage(tr("Image fermée"), 2000);
}

void MainWindow::zoomIn()
{
    scaleImage(1.25);
}

void MainWindow::zoomOut()
{
    scaleImage(0.8);
}

void MainWindow::resetZoom()
{
    m_scaleFactor = 1.0;
    updateImageDisplay();
}

void MainWindow::updateImageDisplay()
{
    if (!m_currentImage.isNull())
    {
        QPixmap pixmap = QPixmap::fromImage(m_currentImage);
        QSize scaledSize = pixmap.size() * m_scaleFactor;
        m_imageLabel->setPixmap(
            pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_imageLabel->adjustSize();

        statusBar()->showMessage(tr("Zoom: %1%").arg(static_cast<int>(m_scaleFactor * 100)), 2000);
    }
}

void MainWindow::scaleImage(double factor)
{
    if (m_currentImage.isNull())
    {
        return;
    }

    m_scaleFactor *= factor;

    // Limiter le zoom entre 10% et 500%
    m_scaleFactor = qBound(0.1, m_scaleFactor, 5.0);

    updateImageDisplay();
}

void MainWindow::createActions()
{
    // Actions Fichier
    m_newAct = new QAction(tr("&Nouveau..."), this);
    m_newAct->setShortcut(QKeySequence::New);
    m_newAct->setStatusTip(tr("Créer une nouvelle image"));
    connect(m_newAct, &QAction::triggered, this, &MainWindow::newImage);

    m_openAct = new QAction(tr("&Ouvrir..."), this);
    m_openAct->setShortcut(QKeySequence::Open);
    m_openAct->setStatusTip(tr("Ouvrir une image existante"));
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openImage);

    m_saveAct = new QAction(tr("&Enregistrer sous..."), this);
    m_saveAct->setShortcut(QKeySequence::SaveAs);
    m_saveAct->setStatusTip(tr("Enregistrer l'image sous un nouveau nom"));
    connect(m_saveAct, &QAction::triggered, this, &MainWindow::saveImage);

    m_closeAct = new QAction(tr("&Fermer"), this);
    m_closeAct->setShortcut(QKeySequence::Close);
    m_closeAct->setStatusTip(tr("Fermer l'image actuelle"));
    connect(m_closeAct, &QAction::triggered, this, &MainWindow::closeImage);

    m_exitAct = new QAction(tr("&Quitter"), this);
    m_exitAct->setShortcut(QKeySequence::Quit);
    m_exitAct->setStatusTip(tr("Quitter l'application"));
    connect(m_exitAct, &QAction::triggered, this, &QWidget::close);

    m_openEpgAct = new QAction(tr("Ouvrir un fichier &EPG..."), this);
    m_openEpgAct->setStatusTip(tr("Ouvrir un fichier EpiGimp (.epg)"));
    connect(m_openEpgAct, &QAction::triggered, this, &MainWindow::openEpg);

    m_saveEpgAct = new QAction(tr("Enregistrer au format &EPG..."), this);
    m_saveEpgAct->setStatusTip(tr("Enregistrer l'image au format EpiGimp (.epg)"));
    connect(m_saveEpgAct, &QAction::triggered, this, &MainWindow::saveAsEpg);

    // Actions Vue
    m_zoomInAct = new QAction(tr("Zoom &Avant"), this);
    m_zoomInAct->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAct->setStatusTip(tr("Agrandir l'image"));
    connect(m_zoomInAct, &QAction::triggered, this, &MainWindow::zoomIn);

    m_zoomOutAct = new QAction(tr("Zoom A&rrière"), this);
    m_zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAct->setStatusTip(tr("Réduire l'image"));
    connect(m_zoomOutAct, &QAction::triggered, this, &MainWindow::zoomOut);

    m_resetZoomAct = new QAction(tr("&Taille réelle"), this);
    m_resetZoomAct->setShortcut(tr("Ctrl+0"));
    m_resetZoomAct->setStatusTip(tr("Afficher l'image à 100%"));
    connect(m_resetZoomAct, &QAction::triggered, this, &MainWindow::resetZoom);
}

void MainWindow::createMenus()
{
    // Menu Fichier
    m_fileMenu = menuBar()->addMenu(tr("&Fichier"));
    m_fileMenu->addAction(m_newAct);
    m_fileMenu->addAction(m_openAct);
    m_fileMenu->addAction(m_openEpgAct);
    m_fileMenu->addAction(m_saveAct);
    m_fileMenu->addAction(m_saveEpgAct);
    m_fileMenu->addAction(m_closeAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAct);

    // Menu Vue
    m_viewMenu = menuBar()->addMenu(tr("&Vue"));
    m_viewMenu->addAction(m_zoomInAct);
    m_viewMenu->addAction(m_zoomOutAct);
    m_viewMenu->addAction(m_resetZoomAct);
}

void MainWindow::saveAsEpg()
{
    if (m_currentImage.isNull())
    {
        QMessageBox::information(this, tr("Information"), tr("Aucune image à sauvegarder."));
        return;
    }

    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString fileName = QFileDialog::getSaveFileName(this, tr("Enregistrer au format EpiGimp"),
                                                    picturesPath, tr("EpiGimp (*.epg)"));

    if (fileName.isEmpty())
    {
        return;
    }

    if (!fileName.endsWith(".epg", Qt::CaseInsensitive))
    {
        fileName += ".epg";
    }

    if (!saveEpgFormat(fileName))
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de sauvegarder le fichier EPG %1")
                                  .arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    m_currentFileName = fileName;
    statusBar()->showMessage(tr("Fichier EPG sauvegardé: %1").arg(fileName), 3000);
}

void MainWindow::openEpg()
{
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Ouvrir un fichier EpiGimp"), picturesPath,
                                     tr("EpiGimp (*.epg);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
    {
        return;
    }

    if (!openEpgFormat(fileName))
    {
        QMessageBox::critical(
            this, tr("Erreur"),
            tr("Impossible de charger le fichier EPG %1\nLe fichier est peut-être corrompu.")
                .arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    m_currentFileName = fileName;
    updateImageDisplay();
    m_scrollArea->setVisible(true);

    const QString message = tr("Fichier EPG chargé: %1 (%2x%3)")
                                .arg(QFileInfo(fileName).fileName())
                                .arg(m_currentImage.width())
                                .arg(m_currentImage.height());

    statusBar()->showMessage(message);
    setWindowTitle(tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
}

bool MainWindow::saveEpgFormat(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);

    // Écrire l'en-tête du format EPG
    out << QString("EPIGIMP");
    out << qint32(1);  // Version du format

    // Écrire les métadonnées
    out << m_currentImage.width();
    out << m_currentImage.height();
    out << qint32(m_currentImage.format());

    // Écrire les données de l'image
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    m_currentImage.save(&buffer, "PNG");

    out << imageData;

    file.close();
    return true;
}

bool MainWindow::openEpgFormat(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    // Lire et vérifier l'en-tête
    QString header;
    in >> header;
    if (header != "EPIGIMP")
    {
        return false;
    }

    qint32 version;
    in >> version;
    if (version != 1)
    {
        return false;
    }

    // Lire les métadonnées
    int width, height;
    qint32 format;
    in >> width >> height >> format;

    // Lire les données de l'image
    QByteArray imageData;
    in >> imageData;

    file.close();

    // Reconstruire l'image
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::ReadOnly);
    m_currentImage.load(&buffer, "PNG");

    return !m_currentImage.isNull();
}
