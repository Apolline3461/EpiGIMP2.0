#include "ui/window.hpp"

#include <QColor>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QStandardPaths>
#include <QStatusBar>

#include <string>

#include "core/Compositor.hpp"
#include "core/Document.hpp"
#include "core/ImageBuffer.hpp"
#include "core/Layer.hpp"
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
    createLayersPanel();
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

    m_addLayerAct = new QAction(tr("Ajouter un calque"), this);
    m_addLayerAct->setStatusTip(tr("Ajouter un nouveau calque vide au document"));
    connect(m_addLayerAct, &QAction::triggered, this, &MainWindow::addNewLayer);

    m_addImageLayerAct = new QAction(tr("Ajouter une image en calque"), this);
    m_addImageLayerAct->setStatusTip(
        tr("Charger une image et l'ajouter en tant que nouveau calque"));
    connect(m_addImageLayerAct, &QAction::triggered, this, &MainWindow::addImageAsLayer);

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
    m_fileMenu->addAction(m_addLayerAct);
    m_fileMenu->addAction(m_addImageLayerAct);
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

void MainWindow::addNewLayer()
{
    if (!m_document)
    {
        // If there's no Document but we have a current image, create a Document from it.
        if (m_currentImage.isNull())
            return;

        const int width = m_currentImage.width();
        const int height = m_currentImage.height();
        ImageBuffer buf(width, height);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const QRgb p = m_currentImage.pixel(x, y);
                const uint8_t r = qRed(p);
                const uint8_t g = qGreen(p);
                const uint8_t b = qBlue(p);
                const uint8_t a = qAlpha(p);
                const uint32_t rgba = (static_cast<uint32_t>(r) << 24) |
                                      (static_cast<uint32_t>(g) << 16) |
                                      (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
                buf.setPixel(x, y, rgba);
            }
        }
        m_document = std::make_unique<Document>(width, height, 72.f);
        auto imgPtr = std::make_shared<ImageBuffer>(buf);
        auto baseLayer =
            std::make_shared<Layer>(1ull, std::string("Background"), imgPtr, true, false, 1.0f);
        m_document->addLayer(baseLayer);
    }
    const int w = m_document->width();
    const int h = m_document->height();
    if (w <= 0 || h <= 0)
        return;

    auto img = std::make_shared<ImageBuffer>(w, h);
    img->fill(0u);  // transparent

    uint64_t id = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch());
    const std::string name = "Layer" + std::to_string(m_document->layerCount());
    auto layer = std::make_shared<Layer>(id, name, img, true, false, 1.0f);
    m_document->addLayer(layer);
    populateLayersList();
    updateImageFromDocument();
}

void MainWindow::addImageAsLayer()
{
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Ajouter une image en nouveau calque"), picturesPath,
        tr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.webp);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
        return;

    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage image = reader.read();

    if (image.isNull())
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de charger l'image %1:\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(reader.errorString()));
        return;
    }

    // If there's no document yet, create one from the loaded image (same behavior as openImage)
    if (!m_document)
    {
        m_currentImage = image;
        m_currentFileName = fileName;

        const int width = m_currentImage.width();
        const int height = m_currentImage.height();
        ImageBuffer buf(width, height);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const QRgb p = m_currentImage.pixel(x, y);
                const uint8_t r = qRed(p);
                const uint8_t g = qGreen(p);
                const uint8_t b = qBlue(p);
                const uint8_t a = qAlpha(p);
                const uint32_t rgba = (static_cast<uint32_t>(r) << 24) |
                                      (static_cast<uint32_t>(g) << 16) |
                                      (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
                buf.setPixel(x, y, rgba);
            }
        }
        m_document = std::make_unique<Document>(width, height, 72.f);
        auto imgPtr = std::make_shared<ImageBuffer>(buf);
        auto layer =
            std::make_shared<Layer>(1ull, std::string("Background"), imgPtr, true, false, 1.0f);
        m_document->addLayer(layer);
        populateLayersList();
        updateImageFromDocument();
        m_scrollArea->setVisible(true);
        QString message = tr("Image chargée et ajoutée comme calque: %1 (%2x%3)")
                              .arg(QFileInfo(fileName).fileName())
                              .arg(m_currentImage.width())
                              .arg(m_currentImage.height());
        statusBar()->showMessage(message);
        setWindowTitle(tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
        return;
    }

    // Create a new transparent buffer matching document size and blit the loaded image at (0,0)
    const int docW = m_document->width();
    const int docH = m_document->height();
    if (docW <= 0 || docH <= 0)
        return;

    auto imgBuf = std::make_shared<ImageBuffer>(docW, docH);
    imgBuf->fill(0u);  // transparent

    // copy pixels from loaded image into the new layer (clamped to document bounds)
    const int copyW = std::min(docW, image.width());
    const int copyH = std::min(docH, image.height());
    for (int y = 0; y < copyH; ++y)
    {
        for (int x = 0; x < copyW; ++x)
        {
            const QRgb p = image.pixel(x, y);
            const uint8_t r = qRed(p);
            const uint8_t g = qGreen(p);
            const uint8_t b = qBlue(p);
            const uint8_t a = qAlpha(p);
            const uint32_t rgba = (static_cast<uint32_t>(r) << 24) |
                                  (static_cast<uint32_t>(g) << 16) |
                                  (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
            imgBuf->setPixel(x, y, rgba);
        }
    }

    uint64_t id = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch());
    const std::string name = "Layer " + std::to_string(m_document->layerCount() + 1);
    auto layer = std::make_shared<Layer>(id, name, imgBuf, true, false, 1.0f);
    m_document->addLayer(layer);
    populateLayersList();
    updateImageFromDocument();
    m_scrollArea->setVisible(true);
    statusBar()->showMessage(
        tr("Image ajoutée en tant que calque: %1").arg(QFileInfo(fileName).fileName()));
}

void MainWindow::createLayersPanel()
{
    m_layersDock = new QDockWidget(tr("Calques"), this);
    m_layersList = new QListWidget(m_layersDock);
    m_layersList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_layersList->setDragDropMode(QAbstractItemView::InternalMove);
    m_layersList->setDefaultDropAction(Qt::MoveAction);
    m_layersList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_layersList, &QListWidget::itemDoubleClicked, this, &MainWindow::onLayerDoubleClicked);
    connect(m_layersList, &QListWidget::customContextMenuRequested, this,
            &MainWindow::onShowLayerContextMenu);
    // detect reorder by monitoring the model signals
    connect(m_layersList->model(), &QAbstractItemModel::rowsMoved, this,
            &MainWindow::onLayersRowsMoved);

    m_layersDock->setWidget(m_layersList);
    addDockWidget(Qt::RightDockWidgetArea, m_layersDock);
}

void MainWindow::populateLayersList()
{
    if (!m_document)
    {
        if (m_layersList)
            m_layersList->clear();
        return;
    }
    m_layersList->clear();
    for (int i = m_document->layerCount() - 1; i >= 0; --i)
    {
        auto layer = m_document->layerAt(i);
        if (!layer)
            continue;

        QListWidgetItem* item = new QListWidgetItem(m_layersList);
        // store the document index for this row
        item->setData(Qt::UserRole, i);
        const bool isBottom = (i == 0);
        const bool isLocked = layer->locked();
        Qt::ItemFlags flags = item->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (!isBottom && !isLocked)
            flags = flags | Qt::ItemIsDragEnabled;
        item->setFlags(flags);
        m_layersList->addItem(item);

        // Create a custom widget for the layer row
        QWidget* row = new QWidget();
        QHBoxLayout* h = new QHBoxLayout(row);
        h->setContentsMargins(4, 2, 4, 2);

        QPushButton* eyeBtn = new QPushButton(row);
        eyeBtn->setFlat(true);
        // initial visibility glyph
        eyeBtn->setText(layer->visible() ? QString::fromUtf8("👁") : QString::fromUtf8("⦻"));
        eyeBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        {
            QFontMetrics fm(eyeBtn->font());
            const int textW = fm.horizontalAdvance(eyeBtn->text());
            const int textH = fm.height();
            const int padW = 8;
            const int padH = 4;
            eyeBtn->setFixedSize(textW + padW, textH + padH);
        }
        QPushButton* lockBtn = new QPushButton(row);
        lockBtn->setFlat(true);
        lockBtn->setText(layer->locked() ? QString::fromUtf8("🔒") : QString::fromUtf8("🔓"));
        lockBtn->setToolTip(tr("Verrouiller/déverrouiller"));
        lockBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        {
            QFontMetrics fm(lockBtn->font());
            const int textW = fm.horizontalAdvance(lockBtn->text());
            const int textH = fm.height();
            const int padW = 8;
            const int padH = 4;
            lockBtn->setFixedSize(textW + padW, textH + padH);
        }

        QLabel* thumb = new QLabel(row);
        thumb->setFixedSize(48, 48);
        thumb->setAlignment(Qt::AlignCenter);

        eyeBtn->setToolTip(tr("Basculer visibilité"));

        QLineEdit* nameEdit = new QLineEdit(QString::fromStdString(layer->name()), row);
        nameEdit->setFrame(false);
        nameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QSlider* opacitySlider = new QSlider(Qt::Horizontal, row);
        opacitySlider->setRange(0, 100);
        opacitySlider->setValue(static_cast<int>(layer->opacity() * 100.0f));
        opacitySlider->setFixedWidth(50);
        opacitySlider->setEnabled(!layer->locked());

        h->addWidget(eyeBtn);
        h->addWidget(lockBtn);
        h->addWidget(nameEdit);
        h->addWidget(opacitySlider);
        h->addWidget(thumb);

        m_layersList->setItemWidget(item, row);

        thumb->setPixmap(createLayerThumbnail(layer, thumb->size()));

        connect(eyeBtn, &QPushButton::clicked, this,
                [this, layer, eyeBtn, thumb]()
                {
                    layer->setVisible(!layer->visible());
                    eyeBtn->setText(layer->visible() ? QString::fromUtf8("👁")
                                                     : QString::fromUtf8("⦻"));
                    updateImageFromDocument();
                    thumb->setPixmap(createLayerThumbnail(layer, thumb->size()));
                });

        connect(lockBtn, &QPushButton::clicked, this,
                [this, layer, lockBtn, nameEdit, opacitySlider, item, isBottom, thumb]()
                {
                    layer->setLocked(!layer->locked());
                    lockBtn->setText(layer->locked() ? QString::fromUtf8("🔒")
                                                     : QString::fromUtf8("🔓"));
                    // disable editing when locked
                    nameEdit->setReadOnly(isBottom || layer->locked());
                    opacitySlider->setEnabled(!layer->locked());
                    // update item flags to reflect dragability
                    Qt::ItemFlags flags = item->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
                    if (!isBottom && !layer->locked())
                        flags = flags | Qt::ItemIsDragEnabled;
                    item->setFlags(flags);
                    updateImageFromDocument();
                    thumb->setPixmap(createLayerThumbnail(layer, thumb->size()));
                });

        connect(opacitySlider, &QSlider::valueChanged, this,
                [this, layer, thumb](int v)
                {
                    layer->setOpacity(static_cast<float>(v) / 100.0f);
                    updateImageFromDocument();
                    thumb->setPixmap(createLayerThumbnail(layer, thumb->size()));
                });

        // make background name read-only or if layer locked
        if (isBottom || layer->locked())
            nameEdit->setReadOnly(true);

        connect(nameEdit, &QLineEdit::editingFinished, this,
                [layer, nameEdit]() { layer->setName(nameEdit->text().toStdString()); });
    }
}

void MainWindow::onLayerItemChanged(QListWidgetItem* item)
{
    if (!m_document || !item)
        return;
    const int idx = item->data(Qt::UserRole).toInt();
    auto layer = m_document->layerAt(idx);
    if (!layer)
        return;
    const bool visible = (item->checkState() == Qt::Checked);
    layer->setVisible(visible);
    updateImageFromDocument();
}

void MainWindow::onLayerDoubleClicked(QListWidgetItem* item)
{
    // Toggle locked on double click
    if (!m_document || !item)
        return;
    const int idx = item->data(Qt::UserRole).toInt();
    auto layer = m_document->layerAt(idx);
    if (!layer)
        return;
    layer->setLocked(!layer->locked());
    // Refresh UI to reflect lock state
    populateLayersList();
    updateImageFromDocument();
}

void MainWindow::onShowLayerContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_layersList->itemAt(pos);
    QMenu menu(this);
    QAction* const mergeDownAct = menu.addAction(tr("Merge Down"));
    QAction* const act = menu.exec(m_layersList->mapToGlobal(pos));
    if (act == mergeDownAct && item && m_document)
    {
        const int idx = item->data(Qt::UserRole).toInt();
        m_document->mergeDown(idx);
        populateLayersList();
        updateImageFromDocument();
    }
}

void MainWindow::onLayersRowsMoved(const QModelIndex& /*parent*/, int /*start*/, int /*end*/,
                                   const QModelIndex& /*destination*/, int /*row*/)
{
    if (!m_document)
        return;
    const int cnt = m_layersList->count();
    std::vector<std::shared_ptr<Layer>> oldLayers;
    oldLayers.reserve(m_document->layerCount());
    for (int i = 0; i < m_document->layerCount(); ++i)
    {
        oldLayers.push_back(m_document->layerAt(i));
    }

    // Rebuild document layers in bottom-to-top order. The visual list is top-first,
    // so iterate the widget items from bottom to top to produce a bottom-first vector.
    std::vector<std::shared_ptr<Layer>> newLayers;
    newLayers.reserve(cnt);
    for (int i = cnt - 1; i >= 0; --i)
    {
        QListWidgetItem* it = m_layersList->item(i);
        if (!it)
            continue;
        const int oldIdx = it->data(Qt::UserRole).toInt();
        if (oldIdx >= 0 && oldIdx < static_cast<int>(oldLayers.size()))
            newLayers.push_back(oldLayers[static_cast<size_t>(oldIdx)]);
    }

    if (!newLayers.empty())
    {
        m_document->setLayers(std::move(newLayers));
        populateLayersList();
        updateImageFromDocument();
    }
}

void MainWindow::updateImageFromDocument()
{
    if (!m_document || m_document->layerCount() == 0)
        return;
    // Compose document into an ImageBuffer
    ImageBuffer outBuf(m_document->width(), m_document->height());
    Compositor comp;
    comp.compose(*m_document, outBuf);

    QImage img(outBuf.width(), outBuf.height(), QImage::Format_ARGB32);
    for (int y = 0; y < outBuf.height(); ++y)
    {
        for (int x = 0; x < outBuf.width(); ++x)
        {
            const uint32_t rgba = outBuf.getPixel(x, y);
            const uint8_t r = static_cast<uint8_t>((rgba >> 24) & 0xFF);
            const uint8_t g = static_cast<uint8_t>((rgba >> 16) & 0xFF);
            const uint8_t b = static_cast<uint8_t>((rgba >> 8) & 0xFF);
            const uint8_t a = static_cast<uint8_t>(rgba & 0xFF);
            img.setPixel(x, y, qRgba(r, g, b, a));
        }
    }
    m_currentImage = img;
    updateImageDisplay();
}

QPixmap MainWindow::createLayerThumbnail(const std::shared_ptr<Layer>& layer,
                                         const QSize& size) const
{
    QPixmap out(size);
    out.fill(Qt::transparent);

    if (!layer || !layer->image())
    {
        QPainter p(&out);
        p.fillRect(out.rect(), QColor(200, 200, 200));
        p.end();
        return out;
    }

    const ImageBuffer& lb = *layer->image();
    if (lb.width() <= 0 || lb.height() <= 0)
        return out;

    QImage img(lb.width(), lb.height(), QImage::Format_ARGB32);
    for (int y = 0; y < lb.height(); ++y)
    {
        for (int x = 0; x < lb.width(); ++x)
        {
            const uint32_t rgba = lb.getPixel(x, y);
            const uint8_t r = static_cast<uint8_t>((rgba >> 24) & 0xFF);
            const uint8_t g = static_cast<uint8_t>((rgba >> 16) & 0xFF);
            const uint8_t b = static_cast<uint8_t>((rgba >> 8) & 0xFF);
            const uint8_t a = static_cast<uint8_t>(rgba & 0xFF);
            img.setPixel(x, y, qRgba(r, g, b, a));
        }
    }

    QPixmap src = QPixmap::fromImage(img);
    QSize target = size;

    // Compute scaled size but do not upscale small images
    QSize scaledSize = src.size();
    if (src.width() > target.width() || src.height() > target.height())
        scaledSize = src.size().scaled(target, Qt::KeepAspectRatio);

    QPixmap scaled = src.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Draw the scaled pixmap centered on a transparent canvas of the target size
    QPixmap canvas(target);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    const int x = (target.width() - scaled.width()) / 2;
    const int y = (target.height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    painter.end();

    if (!layer->visible())
    {
        QImage dim = canvas.toImage();
        QPainter p(&dim);
        p.fillRect(dim.rect(), QColor(0, 0, 0, 120));
        p.end();
        return QPixmap::fromImage(dim);
    }

    return canvas;
}

void MainWindow::onMergeDown()
{
    QListWidgetItem* item = m_layersList->currentItem();
    if (!item || !m_document)
        return;
    const int idx = item->data(Qt::UserRole).toInt();
    m_document->mergeDown(idx);
    populateLayersList();
    updateImageFromDocument();
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
            // Start panning when space (hand mode) is active
            if (me->button() == Qt::LeftButton && m_handMode)
            {
                m_panningActive = true;
                m_lastPanPos = me->pos();
                m_scrollArea->viewport()->setCursor(Qt::ClosedHandCursor);
                return true;
            }

            // If not panning and left button pressed on the viewport, and a layer is selected,
            // start dragging that layer in document coordinates.
            if (me->button() == Qt::LeftButton && !m_handMode && m_document && m_layersList)
            {
                QListWidgetItem* cur = m_layersList->currentItem();
                int pickedIdx = -1;

                // compute doc coord under mouse
                QPoint vpPos = me->pos();
                int hVal = m_scrollArea->horizontalScrollBar()->value();
                int vVal = m_scrollArea->verticalScrollBar()->value();
                const int docX = static_cast<int>((hVal + vpPos.x()) / m_scaleFactor);
                const int docY = static_cast<int>((vVal + vpPos.y()) / m_scaleFactor);

                if (cur)
                {
                    pickedIdx = cur->data(Qt::UserRole).toInt();
                    // Prevent starting a drag if the selected layer is not editable (e.g., locked)
                    if (pickedIdx > 0)
                    {
                        auto selLayer = m_document->layerAt(pickedIdx);
                        if (!selLayer || !selLayer->isEditable())
                            return true;
                    }
                }
                else
                {
                    // pick topmost editable visible layer under cursor (skip background idx 0)
                    for (int i = m_document->layerCount() - 1; i >= 1; --i)
                    {
                        auto lyr = m_document->layerAt(i);
                        if (!lyr || !lyr->visible() || !lyr->image() || !lyr->isEditable())
                            continue;
                        const int lx = docX - lyr->offsetX();
                        const int ly = docY - lyr->offsetY();
                        if (lx < 0 || ly < 0 || lx >= lyr->image()->width() ||
                            ly >= lyr->image()->height())
                            continue;
                        const uint32_t px = lyr->image()->getPixel(lx, ly);
                        const uint8_t a = static_cast<uint8_t>(px & 0xFFu);
                        if (a == 0)
                            continue;
                        pickedIdx = i;
                        // update selection in the layers list to match picked layer
                        for (int j = 0; j < m_layersList->count(); ++j)
                        {
                            auto it = m_layersList->item(j);
                            if (it && it->data(Qt::UserRole).toInt() == pickedIdx)
                            {
                                m_layersList->setCurrentItem(it);
                                break;
                            }
                        }
                        break;
                    }
                }

                if (pickedIdx <= 0)
                    return true;  // no valid layer picked or attempting to pick background

                auto layer = m_document->layerAt(pickedIdx);
                if (layer)
                {
                    m_layerDragActive = true;
                    m_dragLayerIndex = pickedIdx;
                    m_layerDragStartDocPos = QPoint(docX, docY);
                    m_layerDragInitialOffsetX = layer->offsetX();
                    m_layerDragInitialOffsetY = layer->offsetY();
                    return true;
                }
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

            // Handle layer dragging
            if (m_layerDragActive && m_document && m_dragLayerIndex >= 0)
            {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                QPoint vpPos = me->pos();
                int hVal = m_scrollArea->horizontalScrollBar()->value();
                int vVal = m_scrollArea->verticalScrollBar()->value();
                const int docX = static_cast<int>((hVal + vpPos.x()) / m_scaleFactor);
                const int docY = static_cast<int>((vVal + vpPos.y()) / m_scaleFactor);

                const int dx = docX - m_layerDragStartDocPos.x();
                const int dy = docY - m_layerDragStartDocPos.y();

                auto layer = m_document->layerAt(m_dragLayerIndex);
                if (layer)
                {
                    layer->setOffset(m_layerDragInitialOffsetX + dx,
                                     m_layerDragInitialOffsetY + dy);
                    // preserve selection
                    QListWidgetItem* cur = m_layersList->currentItem();
                    int selIdx = cur ? cur->data(Qt::UserRole).toInt() : -1;
                    populateLayersList();
                    // restore selection by matching stored document index
                    if (selIdx >= 0)
                    {
                        for (int i = 0; i < m_layersList->count(); ++i)
                        {
                            QListWidgetItem* it = m_layersList->item(i);
                            if (it && it->data(Qt::UserRole).toInt() == selIdx)
                            {
                                m_layersList->setCurrentItem(it);
                                break;
                            }
                        }
                    }
                    updateImageFromDocument();
                }
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

            if (me->button() == Qt::LeftButton && m_layerDragActive)
            {
                m_layerDragActive = false;
                m_dragLayerIndex = -1;
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

    // If we have a full Document in the UI, save it (preserving separate layers).
    // Otherwise fall back to building a single-layer Document from the current image.
    try
    {
        ZipEpgStorage storage;
        if (m_document && m_document->layerCount() > 0)
        {
            storage.save(*m_document, fileName.toStdString());
        }
        else
        {
            // Build a Document with a single layer and save using ZipEpgStorage
            Document doc(buf.width(), buf.height(), 72.f);
            auto imgPtr = std::make_shared<ImageBuffer>(buf);
            auto layer =
                std::make_shared<Layer>(1ull, std::string("Background"), imgPtr, true, false, 1.0f);
            doc.addLayer(layer);
            storage.save(doc, fileName.toStdString());
        }
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
    auto res = storage.open(fileName.toStdString());
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

    // take ownership of document for editing in UI
    m_document = std::move(res.document);
    populateLayersList();
    updateImageFromDocument();
}
