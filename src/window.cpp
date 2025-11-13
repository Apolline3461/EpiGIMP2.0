#include "window.hpp"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QStandardPaths>
#include <QStatusBar>

#include "epgformat.hpp"
#include "image.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_imageLabel(new QLabel),
      m_scrollArea(new QScrollArea),
      m_scaleFactor(1.0),
      m_fileMenu(nullptr),
      m_viewMenu(nullptr)
{
    // Configuration du QLabel d'image
    m_imageLabel->setBackgroundRole(QPalette::Base);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_imageLabel->setScaledContents(true);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    // Configuration de la zone de défilement
    m_scrollArea->setBackgroundRole(QPalette::Dark);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setVisible(false);
    m_scrollArea->setWidgetResizable(false);

    setCentralWidget(m_scrollArea);

    // Création de l'interface
    createActions();
    createMenus();

    // Barre de statut
    statusBar()->showMessage(tr("Prêt"));

    // Configuration de la fenêtre principale
    setWindowTitle(tr("EpiGimp 2.0"));
    resize(1024, 768);
}

MainWindow::~MainWindow()
{
    // Qt gère automatiquement la destruction des objets enfants
}

void MainWindow::createActions()
{
    // Menu Fichier
    m_newAct = new QAction(tr("&Nouveau..."), this);
    m_newAct->setShortcut(QKeySequence::New);
    m_newAct->setStatusTip(tr("Créer une nouvelle image"));
    connect(m_newAct, &QAction::triggered, [this]() { ImageActions::newImage(this); });

    m_openAct = new QAction(tr("&Ouvrir..."), this);
    m_openAct->setShortcut(QKeySequence::Open);
    m_openAct->setStatusTip(tr("Ouvrir une image existante"));
    connect(m_openAct, &QAction::triggered, [this]() { ImageActions::openImage(this); });

    m_saveAct = new QAction(tr("&Enregistrer sous..."), this);
    m_saveAct->setShortcut(QKeySequence::SaveAs);
    m_saveAct->setStatusTip(tr("Enregistrer l'image sous un nouveau nom"));
    connect(m_saveAct, &QAction::triggered, [this]() { ImageActions::saveImage(this); });

    m_closeAct = new QAction(tr("&Fermer"), this);
    m_closeAct->setShortcut(QKeySequence::Close);
    m_closeAct->setStatusTip(tr("Fermer l'image actuelle"));
    connect(m_closeAct, &QAction::triggered, [this]() { ImageActions::closeImage(this); });

    m_exitAct = new QAction(tr("&Quitter"), this);
    m_exitAct->setShortcut(QKeySequence::Quit);
    m_exitAct->setStatusTip(tr("Quitter l'application"));
    connect(m_exitAct, &QAction::triggered, this, &QWidget::close);

    // Fichiers EPG
    m_openEpgAct = new QAction(tr("Ouvrir un fichier &EPG..."), this);
    m_openEpgAct->setStatusTip(tr("Ouvrir un fichier EpiGimp (.epg)"));
    connect(m_openEpgAct, &QAction::triggered, this, &MainWindow::openEpg);

    m_saveEpgAct = new QAction(tr("Enregistrer au format &EPG..."), this);
    m_saveEpgAct->setStatusTip(tr("Enregistrer l'image au format EpiGimp (.epg)"));
    connect(m_saveEpgAct, &QAction::triggered, this, &MainWindow::saveAsEpg);

    // Menu Vue
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
        return;

    m_scaleFactor *= factor;
    m_scaleFactor = qBound(0.1, m_scaleFactor, 5.0);
    updateImageDisplay();
}

void MainWindow::saveAsEpg()
{
    if (m_currentImage.isNull())
    {
        QMessageBox::information(this, tr("Information"), tr("Aucune image à sauvegarder."));
        return;
    }

    QString picturesPath =
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/untitled.epg";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Enregistrer au format EpiGimp"),
                                                    picturesPath, tr("EpiGimp (*.epg)"));

    if (fileName.isEmpty())
        fileName = "untitled.epg";

    if (!fileName.endsWith(".epg", Qt::CaseInsensitive))
        fileName += ".epg";

    if (!EpgFormat::save(fileName, m_currentImage))
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de sauvegarder le fichier EPG %1.")
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
        return;

    if (!EpgFormat::load(fileName, m_currentImage))
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
