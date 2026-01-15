#include "ui/window.hpp"

#include <QCursor>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStatusBar>

#include <string>

#include "core/ImageBuffer.h"
#include "core/Layer.h"
#include "io/EpgFormat.hpp"
#include "io/EpgJson.hpp"
#include "ui/image.hpp"

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
    // panning: capter les événements de la zone de viewport
    m_scrollArea->viewport()->installEventFilter(this);

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

    // Zoom presets ×0.5, ×1, ×2
    m_zoom05Act = new QAction(tr("Zoom ×0.5"), this);
    m_zoom05Act->setShortcut(QKeySequence("Ctrl+1"));
    m_zoom05Act->setStatusTip(tr("Zoom 0.5x"));
    connect(m_zoom05Act, &QAction::triggered, [this]() { setScaleAndCenter(0.5); });

    m_zoom1Act = new QAction(tr("Zoom ×1"), this);
    m_zoom1Act->setShortcut(QKeySequence("Ctrl+2"));
    m_zoom1Act->setStatusTip(tr("Zoom 1x"));
    connect(m_zoom1Act, &QAction::triggered, [this]() { setScaleAndCenter(1.0); });

    m_zoom2Act = new QAction(tr("Zoom ×2"), this);
    m_zoom2Act->setShortcut(QKeySequence("Ctrl+3"));
    m_zoom2Act->setStatusTip(tr("Zoom 2x"));
    connect(m_zoom2Act, &QAction::triggered, [this]() { setScaleAndCenter(2.0); });
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
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_zoom05Act);
    m_viewMenu->addAction(m_zoom1Act);
    m_viewMenu->addAction(m_zoom2Act);
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

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_scrollArea->viewport())
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && m_handMode)
            {
                m_panningActive = true;
                m_lastPanPos = me->pos();
                m_scrollArea->viewport()->setCursor(Qt::ClosedHandCursor);
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove)
        {
            if (m_panningActive)
            {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                QPoint delta = me->pos() - m_lastPanPos;
                m_lastPanPos = me->pos();
                m_scrollArea->horizontalScrollBar()->setValue(
                    m_scrollArea->horizontalScrollBar()->value() - delta.x());
                m_scrollArea->verticalScrollBar()->setValue(
                    m_scrollArea->verticalScrollBar()->value() - delta.y());
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && m_panningActive)
            {
                m_panningActive = false;
                m_scrollArea->viewport()->setCursor(Qt::OpenHandCursor);
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat())
    {
        m_handMode = true;
        m_scrollArea->viewport()->setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat())
    {
        m_handMode = false;
        m_panningActive = false;
        m_scrollArea->viewport()->setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::scaleImage(double factor)
{
    if (m_currentImage.isNull())
        return;

    const double target = qBound(0.1, m_scaleFactor * factor, 5.0);
    setScaleAndCenter(target);
}

void MainWindow::setScaleAndCenter(double newScale)
{
    if (m_currentImage.isNull())
        return;

    QPixmap pixmap = QPixmap::fromImage(m_currentImage);

    // taille avant
    QSize oldSize = pixmap.size() * m_scaleFactor;

    // position du curseur dans le viewport
    QPoint vpPos = m_scrollArea->viewport()->mapFromGlobal(QCursor::pos());
    QRect vpRect(QPoint(0, 0), m_scrollArea->viewport()->size());

    int hVal = m_scrollArea->horizontalScrollBar()->value();
    int vVal = m_scrollArea->verticalScrollBar()->value();

    double relX = 0.5;
    double relY = 0.5;
    bool hasFocusPoint = false;

    if (vpRect.contains(vpPos) && oldSize.width() > 0 && oldSize.height() > 0)
    {
        hasFocusPoint = true;
        relX = (hVal + vpPos.x()) / static_cast<double>(oldSize.width());
        relY = (vVal + vpPos.y()) / static_cast<double>(oldSize.height());
    }

    m_scaleFactor = qBound(0.1, newScale, 5.0);

    // mise à jour de l'affichage
    updateImageDisplay();

    // nouvelle taille
    QSize newSize = pixmap.size() * m_scaleFactor;

    // ajuster les scrollbars pour conserver le point centré sous le curseur
    if (hasFocusPoint)
    {
        int newH = static_cast<int>(relX * newSize.width()) - vpPos.x();
        int newV = static_cast<int>(relY * newSize.height()) - vpPos.y();
        m_scrollArea->horizontalScrollBar()->setValue(newH);
        m_scrollArea->verticalScrollBar()->setValue(newV);
    }
    else
    {
        // centrer l'image
        m_scrollArea->horizontalScrollBar()->setValue(
            (newSize.width() - m_scrollArea->viewport()->width()) / 2);
        m_scrollArea->verticalScrollBar()->setValue(
            (newSize.height() - m_scrollArea->viewport()->height()) / 2);
    }
}

void MainWindow::saveAsEpg()
{
    if (m_currentImage.isNull())
    {
        QMessageBox::information(this, tr("Information"), tr("Aucune image à sauvegarder."));
        return;
    }

    QString currentPath = QDir::currentPath() + "/untitled.epg";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Enregistrer au format EpiGimp"),
                                                    currentPath, tr("EpiGimp (*.epg)"));

    if (fileName.isEmpty())
        fileName = "untitled.epg";

    if (!fileName.endsWith(".epg", Qt::CaseInsensitive))
        fileName += ".epg";

    // convert QImage -> ImageBuffer
    ImageBuffer buf(m_currentImage.width(), m_currentImage.height());
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
            buf.setPixel(x, y, rgba);
        }
    }

    // Build a Document with a single layer and save using ZipEpgStorage
    Document doc;
    doc.width = buf.width();
    doc.height = buf.height();

    try
    {
        auto imgPtr = std::make_shared<ImageBuffer>(buf);
        auto layer =
            std::make_shared<Layer>(1ull, std::string("Layer 1"), imgPtr, true, false, 1.0f);
        doc.layers.push_back(layer);

        ZipEpgStorage storage;
        storage.save(doc, fileName.toStdString());
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de sauvegarder le fichier EPG %1.\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(QString::fromLocal8Bit(e.what())));
        return;
    }

    m_currentFileName = fileName;
    statusBar()->showMessage(tr("Fichier EPG sauvegardé: %1").arg(fileName), 3000);
}

void MainWindow::openEpg()
{
    QString currentPath = QDir::currentPath();
    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Ouvrir un fichier EpiGimp"), currentPath,
                                     tr("EpiGimp (*.epg);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
        return;

    ZipEpgStorage storage;
    auto res = storage.open(fileName.toStdString());
    if (!res.success || !res.document)
    {
        QString err = QString::fromLocal8Bit(res.errorMessage.empty() ? "Erreur inconnue"
                                                                      : res.errorMessage.c_str());
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de charger le fichier EPG %1\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(err));
        return;
    }

    const auto& doc = *res.document;
    if (doc.layers.empty() || !doc.layers[0]->image())
    {
        QMessageBox::critical(this, tr("Erreur"), tr("Le document ne contient pas d'image."));
        return;
    }

    const ImageBuffer& ib = *doc.layers[0]->image();
    QImage img(ib.width(), ib.height(), QImage::Format_ARGB32);
    for (int y = 0; y < ib.height(); ++y)
    {
        for (int x = 0; x < ib.width(); ++x)
        {
            const uint32_t rgba = ib.getPixel(x, y);
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

    const QString message = tr("Fichier EPG chargé: %1 (%2x%3)")
                                .arg(QFileInfo(fileName).fileName())
                                .arg(m_currentImage.width())
                                .arg(m_currentImage.height());

    statusBar()->showMessage(message);
    setWindowTitle(tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
}
