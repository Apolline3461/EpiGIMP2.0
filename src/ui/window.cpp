#include "ui/window.hpp"

#include <QColorDialog>
#include <QCursor>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStatusBar>
#include <QToolBar>

#include <vector>

#include "app/Command.hpp"
#include "core/BucketFill.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"
#include "io/EpgFormat.hpp"
#include "io/EpgJson.hpp"
#include "ui/image.hpp"

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

    // Configuration de la zone de défilement
    m_scrollArea->setBackgroundRole(QPalette::Dark);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setVisible(false);
    m_scrollArea->setWidgetResizable(false);

    setCentralWidget(m_scrollArea);

    // Création de l'interface
    createActions();
    createMenus();
    // commande: barre d'outils pour sélection
    QToolBar* cmd = addToolBar(tr("Commande"));
    // show icons only and set a reasonable icon size
    cmd->setIconSize(QSize(20, 20));
    cmd->setToolButtonStyle(Qt::ToolButtonIconOnly);
    if (m_selectToggleAct)
        cmd->addAction(m_selectToggleAct);
    if (m_clearSelectionAct)
        cmd->addAction(m_clearSelectionAct);
    if (m_bucketAct)
        cmd->addAction(m_bucketAct);
    if (m_colorPickerAct)
        cmd->addAction(m_colorPickerAct);
    // Add undo/redo to the command toolbar as icons
    if (m_undoAct)
        cmd->addAction(m_undoAct);
    if (m_redoAct)
        cmd->addAction(m_redoAct);
    // connect selection signal
    connect(m_imageLabel, &ImageLabel::selectionFinished, this, &MainWindow::onMouseSelection);
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

    // Undo / Redo actions
    m_undoAct = new QAction(tr("&Annuler"), this);
    m_undoAct->setShortcut(QKeySequence::Undo);
    m_undoAct->setStatusTip(tr("Annuler la dernière action"));
    m_undoAct->setIcon(QIcon(":/icons/undo.svg"));
    m_undoAct->setEnabled(false);
    connect(m_undoAct, &QAction::triggered,
            [this]()
            {
                if (m_history.canUndo())
                {
                    m_history.undo();
                    updateImageDisplay();
                    statusBar()->showMessage(tr("Annuler"), 1000);
                    if (m_undoAct)
                        m_undoAct->setEnabled(m_history.canUndo());
                    if (m_redoAct)
                        m_redoAct->setEnabled(m_history.canRedo());
                }
            });

    m_redoAct = new QAction(tr("&Rétablir"), this);
    m_redoAct->setShortcut(QKeySequence::Redo);
    m_redoAct->setStatusTip(tr("Rétablir la dernière action annulée"));
    m_redoAct->setIcon(QIcon(":/icons/redo.svg"));
    m_redoAct->setEnabled(false);
    connect(m_redoAct, &QAction::triggered,
            [this]()
            {
                if (m_history.canRedo())
                {
                    m_history.redo();
                    updateImageDisplay();
                    statusBar()->showMessage(tr("Rétablir"), 1000);
                    if (m_undoAct)
                        m_undoAct->setEnabled(m_history.canUndo());
                    if (m_redoAct)
                        m_redoAct->setEnabled(m_history.canRedo());
                }
            });

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

    // Sélection rectangulaire
    m_clearSelectionAct = new QAction(tr("Effacer la sélection"), this);
    m_clearSelectionAct->setStatusTip(tr("Supprimer la sélection active"));
    m_clearSelectionAct->setIcon(QIcon(":/icons/clear_selection.svg"));
    connect(m_clearSelectionAct, &QAction::triggered, this, &MainWindow::clearSelection);

    m_selectToggleAct = new QAction(tr("Mode sélection"), this);
    m_selectToggleAct->setCheckable(true);
    m_selectToggleAct->setStatusTip(tr("Activer le mode sélection par souris"));
    m_selectToggleAct->setIcon(QIcon(":/icons/selection.svg"));
    connect(m_selectToggleAct, &QAction::toggled, this, &MainWindow::toggleSelectionMode);

    m_bucketAct = new QAction(tr("Pot de peinture"), this);
    m_bucketAct->setCheckable(true);
    m_bucketAct->setStatusTip(tr("Activer l'outil pot de peinture"));
    m_bucketAct->setIcon(QIcon(":/icons/bucket.svg"));
    connect(m_bucketAct, &QAction::toggled, this,
            [this](bool enabled)
            {
                m_bucketMode = enabled;
                // désactiver le mode sélection si on active le pot
                if (enabled && m_selectToggleAct)
                {
                    m_selectToggleAct->setChecked(false);
                    if (m_imageLabel)
                        m_imageLabel->setSelectionEnabled(false);
                }
                m_scrollArea->viewport()->setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
            });

    // action pour choisir la couleur séparément
    m_colorPickerAct = new QAction(tr("Couleur de remplissage"), this);
    m_colorPickerAct->setStatusTip(tr("Choisir la couleur utilisée par le pot de peinture"));
    m_colorPickerAct->setIcon(QIcon(":/icons/color_picker.svg"));
    connect(m_colorPickerAct, &QAction::triggered, this,
            [this]()
            {
                QColor chosen =
                    QColorDialog::getColor(m_bucketColor, this, tr("Choisir la couleur"));
                if (chosen.isValid())
                {
                    m_bucketColor = chosen;
                    updateColorPickerIcon();
                }
            });
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
    // Undo/Redo in view menu for now
    m_viewMenu->addSeparator();
    if (m_undoAct)
        m_viewMenu->addAction(m_undoAct);
    if (m_redoAct)
        m_viewMenu->addAction(m_redoAct);
    // set initial colored icon
    updateColorPickerIcon();
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
            // Pot de peinture: clic gauche remplit
            if (me->button() == Qt::LeftButton && m_bucketMode)
            {
                bucketFillAt(me->pos());
                return true;
            }
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
    if (event->key() == Qt::Key_Escape && !event->isAutoRepeat())
    {
        clearSelection();
        event->accept();
        return;
    }

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

    const QString currentPath = QDir::currentPath() + "/untitled.epg";
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
    Document doc(buf.width(), buf.height(), 72.f);

    try
    {
        auto imgPtr = std::make_shared<ImageBuffer>(buf);
        auto layer =
            std::make_shared<Layer>(1ull, std::string("Layer 1"), imgPtr, true, false, 1.0f);
        doc.addLayer(layer);

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
    const QString fileName =
        QFileDialog::getOpenFileName(this, tr("Ouvrir un fichier EpiGimp"), currentPath,
                                     tr("EpiGimp (*.epg);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
        return;

    ZipEpgStorage storage;
    const auto res = storage.open(fileName.toStdString());
    if (!res.success || !res.document)
    {
        const QString err = QString::fromLocal8Bit(
            res.errorMessage.empty() ? "Erreur inconnue" : res.errorMessage.c_str());
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de charger le fichier EPG %1\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(err));
        return;
    }

    const Document& doc = *res.document;
    if (doc.layerCount() == 0)
    {
        QMessageBox::critical(this, tr("Erreur"), tr("Le document ne contient pas d'image."));
        return;
    }

    const auto firstLayer = doc.layerAt(0);
    if (!firstLayer || !firstLayer->image())
    {
        QMessageBox::critical(this, tr("Erreur"), tr("Le document ne contient pas d'image."));
        return;
    }
    const ImageBuffer& ib = *firstLayer->image();

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

void MainWindow::onMouseSelection(const QRect& rect)
{
    // rect is in widget space, need to transform to image space
    if (m_currentImage.isNull())
        return;

    // Validate scale factor (should be between 0.1 and 5.0 as per scaleImage())
    if (m_scaleFactor < 0.1)
        return;

    // Transform from widget space to image space by dividing by scale factor
    // Use rounding for better precision instead of truncation
    QRect imageSpaceRect(static_cast<int>(std::round(rect.x() / m_scaleFactor)),
                         static_cast<int>(std::round(rect.y() / m_scaleFactor)),
                         static_cast<int>(std::round(rect.width() / m_scaleFactor)),
                         static_cast<int>(std::round(rect.height() / m_scaleFactor)));

    auto ref = std::make_shared<ImageBuffer>(m_currentImage.width(), m_currentImage.height());
    // Replace previous selection by default (don't accumulate multiple rects)
    m_selection.clear();
    Selection::Rect selRect{imageSpaceRect.x(), imageSpaceRect.y(), imageSpaceRect.width(),
                            imageSpaceRect.height()};
    m_selection.addRect(selRect, ref);
    if (m_imageLabel)
        m_imageLabel->setSelectionRect(rect);
    updateImageDisplay();
}

void MainWindow::updateColorPickerIcon()
{
    if (!m_colorPickerAct)
        return;

    const int sz = 16;
    QPixmap pix(sz, sz);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(QBrush(m_bucketColor));
    p.setPen(Qt::black);
    p.drawRect(0, 0, sz - 1, sz - 1);
    p.end();
    m_colorPickerAct->setIcon(QIcon(pix));
}

void MainWindow::clearSelection()
{
    m_selection.clear();
    if (m_imageLabel)
        m_imageLabel->clearSelectionRect();
    updateImageDisplay();
}

void MainWindow::toggleSelectionMode(bool enabled)
{
    if (m_imageLabel)
        m_imageLabel->setSelectionEnabled(enabled);
    m_selectToggleAct->setChecked(enabled);
    m_scrollArea->viewport()->setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
    // si on active la sélection, désactiver le pot de peinture
    if (enabled && m_bucketAct)
    {
        m_bucketAct->setChecked(false);
        m_bucketMode = false;
    }
}

void MainWindow::bucketFillAt(const QPoint& viewportPos)
{
    if (m_currentImage.isNull())
        return;

    // Calculer la position dans l'image en tenant compte du zoom et des scrollbars
    int hVal = m_scrollArea->horizontalScrollBar()->value();
    int vVal = m_scrollArea->verticalScrollBar()->value();

    const int imgX = static_cast<int>(std::round((hVal + viewportPos.x()) / m_scaleFactor));
    const int imgY = static_cast<int>(std::round((vVal + viewportPos.y()) / m_scaleFactor));

    if (imgX < 0 || imgX >= m_currentImage.width() || imgY < 0 || imgY >= m_currentImage.height())
        return;

    // Build working buffer directly from current image
    ImageBuffer workingBuffer(m_currentImage.width(), m_currentImage.height());
    for (int y = 0; y < m_currentImage.height(); ++y)
    {
        for (int x = 0; x < m_currentImage.width(); ++x)
        {
            const QRgb p = m_currentImage.pixel(x, y);
            const uint8_t r = qRed(p);
            const uint8_t g = qGreen(p);
            const uint8_t b = qBlue(p);
            const uint8_t a = qAlpha(p);
            const uint32_t rgba = (static_cast<uint32_t>(r) << 24) |
                                  (static_cast<uint32_t>(g) << 16) |
                                  (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
            workingBuffer.setPixel(x, y, rgba);
        }
    }

    const uint32_t newColor = (static_cast<uint32_t>(m_bucketColor.red()) << 24) |
                              (static_cast<uint32_t>(m_bucketColor.green()) << 16) |
                              (static_cast<uint32_t>(m_bucketColor.blue()) << 8) |
                              static_cast<uint32_t>(m_bucketColor.alpha());

    // Perform flood fill and get changed pixels directly
    std::vector<std::tuple<int, int, uint32_t>> changedPixels;
    if (m_selection.hasMask())
    {
        if (m_selection.t_at(imgX, imgY) != 0)
        {
            changedPixels = core::floodFillWithinMaskTracked(workingBuffer, *m_selection.mask(),
                                                             imgX, imgY, core::Color{newColor});
        }
        else
        {
            statusBar()->showMessage(tr("Clic hors de la sélection — aucun remplissage"), 2000);
            return;
        }
    }
    else
    {
        changedPixels = core::floodFillTracked(workingBuffer, imgX, imgY, core::Color{newColor});
    }

    if (changedPixels.empty())
    {
        statusBar()->showMessage(tr("Aucun pixel modifié"), 1000);
        return;
    }

    // Build PixelChange vector from tracked changes
    std::vector<app::PixelChange> changes;
    changes.reserve(changedPixels.size());
    for (const auto& [x, y, oldColor] : changedPixels)
    {
        app::PixelChange pc;
        pc.x = x;
        pc.y = y;
        pc.before = oldColor;
        pc.after = newColor;
        changes.push_back(pc);
    }

    // apply function updates the QImage from the given pixel changes
    app::ApplyFn apply =
        [this](std::uint64_t /*layerId*/, const std::vector<app::PixelChange>& ch, bool useBefore)
    {
        for (const auto& c : ch)
        {
            const uint32_t v = useBefore ? c.before : c.after;
            const uint8_t r = static_cast<uint8_t>((v >> 24) & 0xFF);
            const uint8_t g = static_cast<uint8_t>((v >> 16) & 0xFF);
            const uint8_t b = static_cast<uint8_t>((v >> 8) & 0xFF);
            const uint8_t a = static_cast<uint8_t>(v & 0xFF);
            if (c.x >= 0 && c.x < m_currentImage.width() && c.y >= 0 &&
                c.y < m_currentImage.height())
                m_currentImage.setPixel(c.x, c.y, qRgba(r, g, b, a));
        }
        updateImageDisplay();
    };

    // create command, execute redo and push to history
    auto cmd = std::make_unique<app::DataCommand>(0ull, std::move(changes), apply);
    cmd->redo();
    m_history.push(std::move(cmd));

    // update undo/redo enabled state
    if (m_undoAct)
        m_undoAct->setEnabled(m_history.canUndo());
    if (m_redoAct)
        m_redoAct->setEnabled(m_history.canRedo());

    statusBar()->showMessage(tr("Remplissage effectué"), 2000);
}
