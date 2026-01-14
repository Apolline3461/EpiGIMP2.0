#include "ui/window.hpp"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QScrollArea>
#include <QStandardPaths>
#include <QStatusBar>
#include <QToolBar>

#include <string>

#include "core/ImageBuffer.h"
#include "core/Selection.hpp"
#include "io/EpgFormat.hpp"
#include "ui/image.hpp"

// Helpers: convert current QImage to a shared ImageBuffer
static std::shared_ptr<ImageBuffer> qimageToImageBuffer(const QImage& img)
{
    auto buf = std::make_shared<ImageBuffer>(img.width(), img.height());
    for (int y = 0; y < img.height(); ++y)
    {
        for (int x = 0; x < img.width(); ++x)
        {
            const QRgb p = img.pixel(x, y);
            const uint8_t r = qRed(p);
            const uint8_t g = qGreen(p);
            const uint8_t b = qBlue(p);
            const uint8_t a = qAlpha(p);
            const uint32_t rgba = (static_cast<uint32_t>(r) << 24) |
                                  (static_cast<uint32_t>(g) << 16) |
                                  (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
            buf->setPixel(x, y, rgba);
        }
    }
    return buf;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_imageLabel(new ImageLabel),
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

    connect(m_imageLabel, &ImageLabel::selectionFinished, this, &MainWindow::onMouseSelection);

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

    // Sélection rectangulaire
    m_clearSelectionAct = new QAction(tr("Effacer la sélection"), this);
    m_clearSelectionAct->setStatusTip(tr("Supprimer la sélection active"));
    // set icon for clear selection with fallback
    QIcon clearIcon;
    if (QFile::exists(":/icons/clear_selection.svg"))
    {
        clearIcon = QIcon(":/icons/clear_selection.svg");
    }
    else if (QFile::exists("../src/ui/icons/clear_selection.svg"))
    {
        clearIcon = QIcon("../src/ui/icons/clear_selection.svg");
    }
    else if (QFile::exists("src/ui/icons/clear_selection.svg"))
    {
        clearIcon = QIcon("src/ui/icons/clear_selection.svg");
    }
    if (!clearIcon.isNull())
        m_clearSelectionAct->setIcon(clearIcon);
    connect(m_clearSelectionAct, &QAction::triggered, this, &MainWindow::clearSelection);

    m_selectToggleAct = new QAction(tr("Mode sélection"), this);
    m_selectToggleAct->setCheckable(true);
    m_selectToggleAct->setStatusTip(tr("Activer le mode sélection par souris"));
    // set icon from resources with fallbacks to source file if resource missing
    QIcon selIcon;
    if (QFile::exists(":/icons/selection.svg"))
    {
        selIcon = QIcon(":/icons/selection.svg");
    }
    else if (QFile::exists("../src/ui/icons/selection.svg"))
    {
        selIcon = QIcon("../src/ui/icons/selection.svg");
    }
    else if (QFile::exists("src/ui/icons/selection.svg"))
    {
        selIcon = QIcon("src/ui/icons/selection.svg");
    }
    if (!selIcon.isNull())
        m_selectToggleAct->setIcon(selIcon);
    connect(m_selectToggleAct, &QAction::toggled, this, &MainWindow::toggleSelectionMode);
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

    // Menu Commandes (selection actions)
    m_cmdMenu = menuBar()->addMenu(tr("&Commandes"));
    if (m_selectRectAct)
        m_cmdMenu->addAction(m_selectRectAct);
    if (m_selectToggleAct)
        m_cmdMenu->addAction(m_selectToggleAct);
    if (m_clearSelectionAct)
        m_cmdMenu->addAction(m_clearSelectionAct);

    // Toolbar: add selection toggle and clear
    QToolBar* tb = addToolBar(tr("Outils"));
    if (m_selectToggleAct)
        tb->addAction(m_selectToggleAct);
    if (m_clearSelectionAct)
        tb->addAction(m_clearSelectionAct);
}

void MainWindow::clearSelection()
{
    m_selection_.clear();
    updateImageDisplay();
}

void MainWindow::onMouseSelection(const QRect& rect)
{
    if (m_currentImage.isNull())
        return;

    // ignore if selection mode not enabled
    if (!m_selectToggleAct || !m_selectToggleAct->isChecked())
        return;

    // Convert widget coords to image coords using current scale factor
    const double scale = m_scaleFactor;
    int x = static_cast<int>(std::floor(rect.x() / scale));
    int y = static_cast<int>(std::floor(rect.y() / scale));
    int w = static_cast<int>(std::ceil(rect.width() / scale));
    int h = static_cast<int>(std::ceil(rect.height() / scale));

    // Clamp
    x = std::max(0, x);
    y = std::max(0, y);
    if (x + w > m_currentImage.width())
        w = m_currentImage.width() - x;
    if (y + h > m_currentImage.height())
        h = m_currentImage.height() - y;
    if (w <= 0 || h <= 0)
        return;

    // single selection: clear previous
    m_selection_.clear();
    auto ref = qimageToImageBuffer(m_currentImage);
    m_selection_.addRect(x, y, w, h, ref);
    updateImageDisplay();

    // keep selection mode enabled (do not auto-disable)
}

void MainWindow::toggleSelectionMode(bool enabled)
{
    if (enabled)
        m_imageLabel->setCursor(Qt::CrossCursor);
    else
        m_imageLabel->setCursor(Qt::ArrowCursor);
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
        // Prepare image to display and overlay selection if present
        QImage display = m_currentImage;
        if (m_selection_.hasMask())
        {
            const auto& mask = m_selection_.mask();
            if (mask && mask->width() == display.width() && mask->height() == display.height())
            {
                QImage overlay(display.size(), QImage::Format_ARGB32);
                overlay.fill(Qt::transparent);
                for (int y = 0; y < display.height(); ++y)
                {
                    for (int x = 0; x < display.width(); ++x)
                    {
                        const uint8_t t = m_selection_.t_at(x, y);
                        if (t != 0)
                        {
                            // semi-transparent red highlight
                            overlay.setPixel(x, y, qRgba(255, 0, 0, 100));
                        }
                    }
                }

                QPainter p(&display);
                p.setCompositionMode(QPainter::CompositionMode_SourceOver);
                p.drawImage(0, 0, overlay);
                p.end();
            }
        }

        QPixmap pixmap = QPixmap::fromImage(display);
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

    // convert QImage -> ImageBuffer and call core IO
    ImageBuffer buf = [&]()
    {
        ImageBuffer b(m_currentImage.width(), m_currentImage.height());
        for (int y = 0; y < m_currentImage.height(); ++y)
        {
            for (int x = 0; x < m_currentImage.width(); ++x)
            {
                const QRgb p = m_currentImage.pixel(x, y);
                const uint8_t r = qRed(p);
                const uint8_t g = qGreen(p);
                const uint8_t bch = qBlue(p);
                const uint8_t a = qAlpha(p);
                const uint32_t rgba = (static_cast<uint32_t>(r) << 24) |
                                      (static_cast<uint32_t>(g) << 16) |
                                      (static_cast<uint32_t>(bch) << 8) | static_cast<uint32_t>(a);
                b.setPixel(x, y, rgba);
            }
        }
        return b;
    }();

    if (!EpgFormat::save(fileName.toStdString(), buf))
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de sauvegarder le fichier EPG %1.")
                                  .arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    m_currentFileName = fileName;
    statusBar()->showMessage(tr("Fichier EPG sauvegardé: %1").arg(fileName), 3000);

    // If a selection mask exists, save it alongside the EPG as "<file>.sel"
    if (m_selection_.hasMask())
    {
        const auto& mask = m_selection_.mask();
        if (mask)
        {
            const std::string selName = fileName.toStdString() + ".sel";
            if (!EpgFormat::save(selName, *mask))
            {
                QMessageBox::warning(
                    this, tr("Attention"),
                    tr("Impossible de sauvegarder la sélection: %1")
                        .arg(QDir::toNativeSeparators(QString::fromStdString(selName))));
            }
        }
    }
}

void MainWindow::openEpg()
{
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Ouvrir un fichier EpiGimp"), picturesPath,
                                     tr("EpiGimp (*.epg);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
        return;

    ImageBuffer buf(1, 1);
    if (!EpgFormat::load(fileName.toStdString(), buf))
    {
        QMessageBox::critical(
            this, tr("Erreur"),
            tr("Impossible de charger le fichier EPG %1\nLe fichier est peut-être corrompu.")
                .arg(QDir::toNativeSeparators(fileName)));
        return;
    }
    // convert ImageBuffer -> QImage
    QImage img(buf.width(), buf.height(), QImage::Format_ARGB32);
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

    m_currentImage = img;
    m_currentFileName = fileName;
    updateImageDisplay();
    m_scrollArea->setVisible(true);

    // Try to load an accompanying selection file "<file>.sel"
    {
        ImageBuffer maskBuf(buf.width(), buf.height());
        const std::string selName = fileName.toStdString() + ".sel";
        if (EpgFormat::load(selName, maskBuf))
        {
            m_selection_.setMask(std::make_shared<ImageBuffer>(maskBuf));
        }
        else
        {
            m_selection_.clear();
        }
    }

    const QString message = tr("Fichier EPG chargé: %1 (%2x%3)")
                                .arg(QFileInfo(fileName).fileName())
                                .arg(m_currentImage.width())
                                .arg(m_currentImage.height());

    statusBar()->showMessage(message);
    setWindowTitle(tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
}
