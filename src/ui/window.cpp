#include "ui/window.hpp"

#include <QActionGroup>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>

#include "app/commands/CommandUtils.hpp"
#include "common/Geometry.hpp"
#include "core/Document.hpp"
#include "core/Layer.hpp"
#include "ui/CanvasWidget.hpp"
#include "ui/ImageConversion.hpp"
#include "ui/Render.hpp"

MainWindow::MainWindow(app::AppService& svc, QWidget* parent) : QMainWindow(parent), svc_(svc)
{
    canvas_ = new CanvasWidget(this);
    canvas_->setObjectName("canvas");
    canvas_->setFocusPolicy(Qt::StrongFocus);
    setCentralWidget(canvas_);

    resize(1080, 720);
    // Interface creation
    createActions();
    m_toolColor = QColor(0, 0, 0, 255);
    updateColorPickerIcon();
    createMenus();
    createToolBar();
    if (canvas_)
        canvas_->setPencilColor(m_toolColor);
    createLayersPanel();

    connect(canvas_, &CanvasWidget::selectionFinishedDoc, this,
            [this](common::Rect r) { app().setSelectionRect(r); });

    connect(canvas_, &CanvasWidget::clickedDoc, this,
            [this](common::Point p)
            {
                if (!app().hasDocument())
                    return;
                if (m_pickMode)
                {
                    const uint32_t rgba = app().pickColorAt(p);
                    const uint8_t r = static_cast<uint8_t>((rgba >> 24) & 0xFFu);
                    const uint8_t g = static_cast<uint8_t>((rgba >> 16) & 0xFFu);
                    const uint8_t b = static_cast<uint8_t>((rgba >> 8) & 0xFFu);
                    const uint8_t a = static_cast<uint8_t>(rgba & 0xFFu);

                    m_toolColor = QColor(r, g, b, a);
                    updateColorPickerIcon();
                    if (canvas_)
                        canvas_->setPencilColor(m_toolColor);
                    if (m_pickAct)
                        m_pickAct->setChecked(false);
                    return;
                }
                if (!m_bucketMode)
                    return;
                const uint32_t newColor = (static_cast<uint32_t>(m_toolColor.red()) << 24) |
                                          (static_cast<uint32_t>(m_toolColor.green()) << 16) |
                                          (static_cast<uint32_t>(m_toolColor.blue()) << 8) |
                                          static_cast<uint32_t>(m_toolColor.alpha());
                try
                {
                    app().bucketFill(p, newColor);
                }
                catch (std::exception& e)
                {
                    statusBar()->showMessage(e.what(), 2000);
                }
            });

    connect(
        canvas_, &CanvasWidget::beginStroke, this,
        [this](common::Point p)
        {
            const bool pencilOn = (m_pencilAct && m_pencilAct->isChecked());
            const bool eraserOn = (m_eraseAct && m_eraseAct->isChecked());
            if (!pencilOn && !eraserOn)
                return;

            app::ToolParams params;
            params.tool = eraserOn ? app::ToolKind::Eraser : app::ToolKind::Pencil;

            if (params.tool == app::ToolKind::Eraser)
            {
                params.size = (m_eraseSizeSpin) ? m_eraseSizeSpin->value() : 8;
                params.opacity = (m_eraseOpacitySpin)
                                     ? (static_cast<float>(m_eraseOpacitySpin->value()) / 100.F)
                                     : 1.F;
            }
            else
            {
                params.size = (m_pencilSizeSpin) ? m_pencilSizeSpin->value() : 8;
                params.opacity = (m_pencilOpacitySpin)
                                     ? (static_cast<float>(m_pencilOpacitySpin->value()) / 100.F)
                                     : 1.F;
                params.color = (static_cast<uint32_t>(m_toolColor.red()) << 24) |
                               (static_cast<uint32_t>(m_toolColor.green()) << 16) |
                               (static_cast<uint32_t>(m_toolColor.blue()) << 8) |
                               static_cast<uint32_t>(m_toolColor.alpha());
            }
            try
            {
                app().beginStroke(params, p);
            }
            catch (const std::exception& e)
            {
                // Option: statusBar message
                if (statusBar())
                    statusBar()->showMessage(tr("Impossible de dessiner: %1").arg(e.what()), 2000);
            }
        });

    connect(canvas_, &CanvasWidget::moveStroke, this,
            [this](common::Point p)
            {
                const bool pencilOn = (m_pencilAct && m_pencilAct->isChecked());
                const bool eraserOn = (m_eraseAct && m_eraseAct->isChecked());
                if (!pencilOn && !eraserOn)
                    return;
                try
                {
                    app().moveStroke(p);
                }
                catch (std::exception& e)
                {
                    statusBar()->showMessage(e.what(), 2000);
                }
            });

    connect(canvas_, &CanvasWidget::endStroke, this,
            [this]()
            {
                const bool pencilOn = (m_pencilAct && m_pencilAct->isChecked());
                const bool eraserOn = (m_eraseAct && m_eraseAct->isChecked());
                if (!pencilOn && !eraserOn)
                    return;
                try
                {
                    app().endStroke();
                }
                catch (std::exception& e)
                {
                    statusBar()->showMessage(e.what(), 2000);
                }
            });

    connect(
        canvas_, &CanvasWidget::beginDragDoc, this,
        [this](common::Point p)
        {
            if (!m_moveLayerMode || !app().hasDocument() || !canvas_)
                return;

            auto idxOpt = currentLayerIndexFromSelection();
            if (!idxOpt || *idxOpt == 0)
                return;

            const std::size_t idx = *idxOpt;
            auto layer = app().document().layerAt(idx);
            if (!layer || !layer->image() || layer->locked())
                return;

            // hit test local layer
            const int lx = p.x - layer->offsetX();
            const int ly = p.y - layer->offsetY();
            if (lx < 0 || ly < 0 || lx >= layer->image()->width() || ly >= layer->image()->height())
                return;

            // drag state
            m_dragLayerActive = true;
            m_dragLayerIdx = idx;
            m_dragStartDoc = p;
            m_dragStartOffset = {layer->offsetX(), layer->offsetY()};

            // layer image for preview (once)
            m_dragLayerImage = ImageConversion::imageBufferToQImage(
                *layer->image(), QImage::Format_ARGB32_Premultiplied);

            // base render without this layer (once)
            const bool oldVis = layer->visible();
            layer->setVisible(false);
            m_dragBaseImage = Renderer::render(app().document());
            layer->setVisible(oldVis);

            canvas_->setImage(m_dragBaseImage);
            canvas_->setDragLayerPreview(m_dragLayerImage, m_dragStartOffset.x,
                                         m_dragStartOffset.y);

            // update yellow overlay to match preview
            canvas_->setLayerRectOverlay(common::Rect{m_dragStartOffset.x, m_dragStartOffset.y,
                                                      m_dragLayerImage.width(),
                                                      m_dragLayerImage.height()});
        });

    connect(canvas_, &CanvasWidget::dragDoc, this,
            [this](common::Point p)
            {
                if (!m_dragLayerActive || !canvas_)
                    return;

                const common::Point delta{p.x - m_dragStartDoc.x, p.y - m_dragStartDoc.y};
                const int newX = m_dragStartOffset.x + delta.x;
                const int newY = m_dragStartOffset.y + delta.y;

                // no render here
                canvas_->setDragLayerPos(newX, newY);
                canvas_->setLayerRectOverlay(
                    common::Rect{newX, newY, m_dragLayerImage.width(), m_dragLayerImage.height()});
            });

    connect(canvas_, &CanvasWidget::endDragDoc, this,
            [this](common::Point p)
            {
                if (!m_dragLayerActive || !canvas_ || !app().hasDocument())
                    return;

                const common::Point delta{p.x - m_dragStartDoc.x, p.y - m_dragStartDoc.y};
                const int newX = m_dragStartOffset.x + delta.x;
                const int newY = m_dragStartOffset.y + delta.y;

                // stop preview BEFORE pushing (documentChanged will re-render normal doc)
                m_dragLayerActive = false;
                canvas_->clearDragLayerPreview();

                // clear caches
                m_dragBaseImage = QImage();
                m_dragLayerImage = QImage();

                // undoable commit
                app().moveLayer(m_dragLayerIdx, newX, newY);
            });

    app().documentChanged.connect(
        [this]()
        {
            refreshUIAfterDocChange();
            if (app().hasDocument())
                setDirty(app().isDirty());
        });

    refreshUIAfterDocChange();
}

void MainWindow::showShortcutHelpDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Raccourcis clavier"));
    dlg.setModal(true);
    dlg.resize(640, 520);

    auto* layout = new QVBoxLayout(&dlg);

    auto* text = new QTextEdit(&dlg);
    text->setReadOnly(true);
    text->setAcceptRichText(false);

    // Police monospace pour aligner proprement
    QFont f = text->font();
    f.setFamily(QStringLiteral("Monospace"));
    f.setStyleHint(QFont::Monospace);
    text->setFont(f);

    auto addSection = [&](const QString& title, const QList<const QAction*>& actions)
    {
        QString out;
        out += QStringLiteral("\n");
        out += title.toUpper() + QStringLiteral("\n");
        out += QStringLiteral("----------------------------------------\n");
        for (const QAction* a : actions)
        {
            if (!a)
                continue;
            // ignore actions without shortcut if you want:
            // if (a->shortcut().isEmpty()) continue;

            out += formatActionShortcut(a) + QStringLiteral("\n");
        }
        out += QStringLiteral("\n");
        return out;
    };

    QString content;
    content += tr("Astuce : appuie sur Ctrl+/ n'importe où pour ouvrir cette aide.\n");

    // --- FILE
    content +=
        addSection(tr("Fichier"), {m_newAct, m_openAct, m_saveAct, m_closeAct, m_exitAct,
                                   m_openEpgAct, m_saveEpgAct, m_addLayerAct, m_addImageLayerAct});

    // --- VIEW / ZOOM
    content += addSection(tr("Vue / Zoom"), {m_zoomInAct, m_zoomOutAct, m_resetZoomAct, m_zoom05Act,
                                             m_zoom1Act, m_zoom2Act});

    // --- EDIT / HISTORY
    content += addSection(tr("Historique"), {m_undoAct, m_redoAct});

    // --- TOOLS
    content += addSection(tr("Outils"),
                          {m_moveLayerAct, m_selectToggleAct, m_clearSelectionAct, m_bucketAct,
                           m_pickAct, m_pencilAct, m_eraseAct, m_colorPickerAct});

    // --- LAYERS (actions)
    content += addSection(
        tr("Calques"),
        {
            m_layerUpAct, m_layerDownAct
            // Ajoute ici tes futures actions clavier : MergeDown, Delete, Lock, Visible, Rename...
        });

    // --- CANVAS keyboard (pas des QActions actuellement => on l’écrit en dur)
    // (Tu peux plus tard convertir en QActions si tu veux)
    // content += QStringLiteral("\n");
    // content += tr("CANVAS (clavier)\n");
    // content += QStringLiteral("----------------------------------------\n");
    // content += tr("Flèches : déplacer le curseur (si tu ajoutes le curseur clavier)\n");
    // content += tr("Entrée/Espace : commencer/terminer un trait (si implémenté)\n");
    // content += tr("Échap : annuler (sélection/trait)\n");
    // content += tr("+ / - : zoomer / dézoomer (si tu ajoutes les raccourcis)\n");
    // content += tr("0 : zoom 100%% (si tu ajoutes le raccourci)\n");

    text->setPlainText(content);

    layout->addWidget(text);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    layout->addWidget(buttons);

    // Focus direct pour navigation clavier
    text->setFocus();

    dlg.exec();
}

void MainWindow::zoomIn()
{
    if (!canvas_ || !canvas_->hasImage())
        return;
    canvas_->setScale(canvas_->scale() * 1.25);
}

void MainWindow::zoomOut()
{
    if (!canvas_ || !canvas_->hasImage())
        return;
    canvas_->setScale(canvas_->scale() / 1.25);
}

void MainWindow::resetZoom()
{
    if (!canvas_)
        return;
    canvas_->setScale(1.0);
}

void MainWindow::clearSelection()
{
    if (!app().hasDocument())
        return;
    app().clearSelectionRect();
}

void MainWindow::toggleSelectionMode(bool enabled)
{
    if (enabled)
    {
        if (m_moveLayerAct)
            m_moveLayerAct->setChecked(false);
        if (m_bucketAct)
            m_bucketAct->setChecked(false);
        if (m_pickAct)
            m_pickAct->setChecked(false);
        if (m_pencilAct)
            m_pencilAct->setChecked(false);
        if (m_eraseAct)
            m_eraseAct->setChecked(false);

        m_bucketMode = false;
        m_pickMode = false;

        if (canvas_)
            canvas_->setPencilEnable(false);
    }
    if (canvas_)
        canvas_->setSelectionEnable(enabled);
    if (m_selectToggleAct)
        m_selectToggleAct->setChecked(enabled);
}

// GCOVR_EXCL_START

void MainWindow::newImage()
{
    if (!confirmDiscardIfDirty(tr("Créer une nouvelle image"), false))
        return;
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Nouvelle image"));

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QSpinBox* wSpin = new QSpinBox(&dialog);
    wSpin->setRange(1, 10000);
    wSpin->setValue(800);

    QSpinBox* hSpin = new QSpinBox(&dialog);
    hSpin->setRange(1, 10000);
    hSpin->setValue(600);

    auto* wLay = new QHBoxLayout();
    wLay->addWidget(new QLabel(tr("Largeur (px):"), &dialog));
    wLay->addWidget(wSpin);
    wLay->addStretch();

    auto* hLay = new QHBoxLayout();
    hLay->addWidget(new QLabel(tr("Hauteur (px):"), &dialog));
    hLay->addWidget(hSpin);
    hLay->addStretch();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // --- Background controls ---
    auto* bgLayout = new QHBoxLayout();

    auto* transparentBgCheck = new QCheckBox(tr("Créer un calque transparent"), &dialog);
    transparentBgCheck->setChecked(false);

    auto* colorBtn = new QPushButton(tr("Choisir la couleur"), &dialog);
    auto* colorPreview = new QLabel(&dialog);
    colorPreview->setFixedSize(24, 24);
    colorPreview->setFrameStyle(QFrame::Box | QFrame::Plain);
    colorPreview->setAutoFillBackground(true);

    // default: white background
    QColor canvasColor = QColor(255, 255, 255, 255);

    auto updatePreview = [&]()
    {
        QPalette pal = colorPreview->palette();
        pal.setColor(QPalette::Window, canvasColor);
        colorPreview->setPalette(pal);
    };
    updatePreview();

    auto updateBgUiEnabled = [&]()
    {
        const bool isTransparent = transparentBgCheck->isChecked();
        colorBtn->setEnabled(!isTransparent);
        colorPreview->setEnabled(!isTransparent);
    };
    updateBgUiEnabled();

    connect(transparentBgCheck, &QCheckBox::toggled, &dialog, [&](bool) { updateBgUiEnabled(); });

    connect(colorBtn, &QPushButton::clicked, &dialog,
            [&]()
            {
                QColor c =
                    QColorDialog::getColor(canvasColor, &dialog, tr("Choisir la couleur du fond"));
                if (c.isValid())
                {
                    canvasColor = c;
                    updatePreview();
                }
            });

    bgLayout->addWidget(transparentBgCheck);
    bgLayout->addSpacing(8);
    bgLayout->addWidget(colorBtn);
    bgLayout->addWidget(colorPreview);
    bgLayout->addStretch();

    mainLayout->addLayout(bgLayout);
    mainLayout->addLayout(wLay);
    mainLayout->addLayout(hLay);
    mainLayout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;

    m_currentFileName.clear();
    std::uint32_t bgColor = common::colors::White;

    if (transparentBgCheck->isChecked())
    {
        bgColor = common::colors::Transparent;
    }
    else
    {
        bgColor = (static_cast<uint32_t>(canvasColor.red()) << 24) |
                  (static_cast<uint32_t>(canvasColor.green()) << 16) |
                  (static_cast<uint32_t>(canvasColor.blue()) << 8) |
                  static_cast<uint32_t>(canvasColor.alpha());
    }

    app().newDocument({wSpin->value(), hSpin->value()}, 72.f, bgColor);
    setWindowTitle(tr("Sans titre - EpiGimp 2.0"));
    statusBar()->showMessage(
        tr("Nouvelle image créée: %1x%2").arg(wSpin->value()).arg(hSpin->value()), 2000);
    setDirty(false);
}

void MainWindow::openImage()
{
    if (!confirmDiscardIfDirty(tr("Ouvrir une image"), false))
        return;
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Ouvrir une image"), picturesPath,
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

    m_currentFileName = fileName;

    app().newDocument({image.width(), image.height()}, 72.f, common::colors::Transparent);

    ImageBuffer buf = ImageConversion::qImageToImageBuffer(image, image.width(), image.height());
    app().replaceBackgroundWithImage(buf, QFileInfo(fileName).baseName().toStdString());

    setWindowTitle(tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
    statusBar()->showMessage(tr("Image chargée: %1").arg(QFileInfo(fileName).fileName()), 2000);
    setDirty(false);
}

void MainWindow::saveImage()
{
    if (!app().hasDocument())
    {
        QMessageBox::information(this, tr("Information"), tr("Aucune image à sauvegarder."));
        return;
    }

    const QString startDir =
        m_currentFileName.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
            : QFileInfo(m_currentFileName).absolutePath();

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Exporter l'image"), startDir + "/untitled.png",
        tr("PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
        return;

    try
    {
        app().exportImage(fileName.toStdString());
        m_currentFileName = fileName;
        statusBar()->showMessage(tr("Image exportée: %1").arg(QFileInfo(fileName).fileName()),
                                 3000);
        setDirty(false);
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible d'exporter %1.\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(QString::fromLocal8Bit(e.what())));
    }
}
// GCOVR_EXCL_STOP

void MainWindow::closeImage()
{
    if (!confirmDiscardIfDirty(tr("Fermer le document"), true))
        return;
    if (!app().hasDocument())
        return;
    app().closeDocument();
    setWindowTitle(tr("EpiGimp 2.0"));
    statusBar()->showMessage(tr("Image fermée"), 2000);
    setDirty(false);
}

// GCOVR_EXCL_START

void MainWindow::openEpg()
{
    if (!confirmDiscardIfDirty(tr("Créer une fichier EPG"), true))
        return;
    const QString startDir =
        m_currentFileName.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
            : QFileInfo(m_currentFileName).absolutePath();

    const QString fileName =
        QFileDialog::getOpenFileName(this, tr("Ouvrir un fichier EpiGimp"), startDir,
                                     tr("EpiGimp (*.epg);;Tous les fichiers (*)"));

    if (fileName.isEmpty())
        return;

    try
    {
        app().open(fileName.toStdString());
        m_currentFileName = fileName;

        setWindowTitle(tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
        statusBar()->showMessage(tr("Fichier EPG chargé: %1").arg(QFileInfo(fileName).fileName()),
                                 3000);
        setDirty(false);
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible d'ouvrir %1.\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(QString::fromLocal8Bit(e.what())));
    }
}

void MainWindow::saveAsEpg()
{
    if (!app().hasDocument())
    {
        QMessageBox::information(this, tr("Information"), tr("Aucune image à sauvegarder."));
        return;
    }

    const QString startDir =
        m_currentFileName.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
            : QFileInfo(m_currentFileName).absolutePath();

    QString fileName =
        QFileDialog::getSaveFileName(this, tr("Enregistrer au format EpiGimp"),
                                     startDir + "/untitled.epg", tr("EpiGimp (*.epg)"));

    if (fileName.isEmpty())
        return;

    if (!fileName.endsWith(".epg", Qt::CaseInsensitive))
        fileName += ".epg";

    try
    {
        app().save(fileName.toStdString());
        m_currentFileName = fileName;

        setWindowTitle(tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
        statusBar()->showMessage(
            tr("Fichier EPG sauvegardé: %1").arg(QFileInfo(fileName).fileName()), 3000);
        setDirty(false);
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de sauvegarder %1.\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(QString::fromLocal8Bit(e.what())));
    }
}

void MainWindow::addNewLayer()
{
    if (!app().hasDocument())
    {
        QMessageBox::information(
            this, tr("Info"), tr("Aucun document. Charge une image ou crée un nouveau document."));
        return;
    }

    const std::string name = "Layer " + std::to_string(app().document().layerCount());

    app::LayerSpec spec;
    spec.name = name;
    spec.visible = true;
    spec.locked = false;
    spec.opacity = 1.f;

    const int defW = app().document().width();
    const int defH = app().document().height();

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Ajouter un calque"));
    auto* dlgLayout = new QVBoxLayout(&dlg);

    // --- Size row ---
    auto* sizeLayout = new QHBoxLayout();
    auto* wLabel = new QLabel(tr("Largeur:"), &dlg);
    auto* wSpin = new QSpinBox(&dlg);
    wSpin->setRange(1, 10000);
    wSpin->setValue(defW);

    auto* hLabel = new QLabel(tr("Hauteur:"), &dlg);
    auto* hSpin = new QSpinBox(&dlg);
    hSpin->setRange(1, 10000);
    hSpin->setValue(defH);

    sizeLayout->addWidget(wLabel);
    sizeLayout->addWidget(wSpin);
    sizeLayout->addSpacing(8);
    sizeLayout->addWidget(hLabel);
    sizeLayout->addWidget(hSpin);
    dlgLayout->addLayout(sizeLayout);

    // --- Color / transparent row ---
    auto* colorLayout = new QHBoxLayout();

    // New behavior: default color always defined, checkbox is "transparent layer"
    auto* transparentCheck = new QCheckBox(tr("Créer un calque transparent"), &dlg);

    auto* colorBtn = new QPushButton(tr("Choisir la couleur"), &dlg);
    auto* colorPreview = new QLabel(&dlg);
    colorPreview->setFixedSize(24, 24);
    colorPreview->setFrameStyle(QFrame::Box | QFrame::Plain);
    colorPreview->setAutoFillBackground(true);

    QColor chosenColor(255, 255, 255, 255);  // default fill: white opaque

    auto updatePreview = [&]()
    {
        if (transparentCheck->isChecked())
        {
            // show "transparent" preview (simple: transparent bg)
            QPalette pal = colorPreview->palette();
            pal.setColor(QPalette::Window, Qt::transparent);
            colorPreview->setPalette(pal);
        }
        else
        {
            QPalette pal = colorPreview->palette();
            pal.setColor(QPalette::Window, chosenColor);
            colorPreview->setPalette(pal);
        }
        colorPreview->update();
    };

    updatePreview();

    QObject::connect(colorBtn, &QPushButton::clicked, &dlg,
                     [&]()
                     {
                         QColor c =
                             QColorDialog::getColor(chosenColor, &dlg, tr("Choisir la couleur"));
                         if (c.isValid())
                         {
                             chosenColor = c;
                             updatePreview();
                         }
                     });

    QObject::connect(transparentCheck, &QCheckBox::toggled, &dlg,
                     [&](bool on)
                     {
                         colorBtn->setEnabled(!on);
                         updatePreview();
                     });

    colorLayout->addWidget(transparentCheck);
    colorLayout->addStretch();
    colorLayout->addWidget(colorBtn);
    colorLayout->addWidget(colorPreview);
    dlgLayout->addLayout(colorLayout);

    // --- Buttons row ---
    auto* buttons = new QHBoxLayout();
    buttons->addStretch();
    auto* okBtn = new QPushButton(tr("OK"), &dlg);
    auto* cancelBtn = new QPushButton(tr("Annuler"), &dlg);
    buttons->addWidget(okBtn);
    buttons->addWidget(cancelBtn);
    dlgLayout->addLayout(buttons);

    QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    const int w = wSpin->value();
    const int h = hSpin->value();
    if (w <= 0 || h <= 0)
        return;

    spec.width = w;
    spec.height = h;

    if (transparentCheck->isChecked())
    {
        spec.color = common::colors::Transparent;
    }
    else
    {
        spec.color = (static_cast<uint32_t>(chosenColor.red()) << 24) |
                     (static_cast<uint32_t>(chosenColor.green()) << 16) |
                     (static_cast<uint32_t>(chosenColor.blue()) << 8) |
                     static_cast<uint32_t>(chosenColor.alpha());
    }

    app().addLayer(spec);
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

    if (!app().hasDocument())
    {
        m_currentFileName = fileName;
        app().newDocument({image.width(), image.height()}, 72.F);
        setWindowTitle(tr("%1 - EpiGimp 2.0").arg(QFileInfo(fileName).fileName()));
    }

    // Create a new transparent buffer matching document size and blit the loaded image at (0,0)

    ImageBuffer buf = ImageConversion::qImageToImageBuffer(image, image.width(), image.height());

    app().addImageLayer(buf, QFileInfo(fileName).baseName().toStdString());
    statusBar()->showMessage(
        tr("Image ajoutée en tant que calque: %1").arg(QFileInfo(fileName).fileName()));
}

void MainWindow::onLayerDoubleClicked(QListWidgetItem* item)
{
    // Double-click on a layer item starts inline editing of the name (if editable)
    if (!app().hasDocument() || !item)
        return;
    const auto layerId = static_cast<std::uint64_t>(item->data(Qt::UserRole).toULongLong());
    const auto idxOpt = app::commands::findLayerIndexById(app().document(), layerId);
    if (!idxOpt)
        return;
    const std::size_t idx = *idxOpt;
    if (idx == 0)
        return;

    auto layer = app().document().layerAt(idx);
    if (!layer || layer->locked())
        return;

    bool ok = false;
    const QString current = QString::fromStdString(layer->name());
    const QString text = QInputDialog::getText(this, tr("Renommer le calque"), tr("Nom:"),
                                               QLineEdit::Normal, current, &ok);
    if (ok)
        app().setLayerName(idx, text.toStdString());
}

void MainWindow::moveLayerUp()
{
    if (!app().hasDocument() || !m_layersList)
        return;

    auto* cur = m_layersList->currentItem();
    if (!cur)
        return;

    const auto layerId = static_cast<std::uint64_t>(cur->data(Qt::UserRole).toULongLong());
    const std::optional<size_t> from = app::commands::findLayerIndexById(app().document(), layerId);
    if (!from.has_value())
        return;
    const auto n = app().document().layerCount();
    if (*from >= n - 1)
        return;

    const auto to = *from + 1;  // doc index : plus grand = plus au-dessus
    m_pendingSelectLayerId_ = layerId;
    app().reorderLayer(*from, to);
}

void MainWindow::moveLayerDown()
{
    if (!app().hasDocument() || !m_layersList)
        return;

    auto* cur = m_layersList->currentItem();
    if (!cur)
        return;

    const auto layerId = static_cast<std::uint64_t>(cur->data(Qt::UserRole).toULongLong());
    const std::optional<size_t> from = app::commands::findLayerIndexById(app().document(), layerId);

    if (!from.has_value() || *from == 0)
        return;

    const auto to = *from - 1;
    m_pendingSelectLayerId_ = layerId;
    app().reorderLayer(*from, to);
}

void MainWindow::onShowLayerContextMenu(const QPoint& pos)
{
    if (!m_layersList || !app().hasDocument())
        return;

    QListWidgetItem* item = m_layersList->itemAt(pos);
    if (!item)
        item = m_layersList->currentItem();
    if (!item)
        return;

    const auto layerId = static_cast<std::uint64_t>(item->data(Qt::UserRole).toULongLong());
    const auto idxOpt = app::commands::findLayerIndexById(app().document(), layerId);
    if (!idxOpt.has_value())
        return;
    const size_t idx = idxOpt.value();
    auto layer = app().document().layerAt(idx);

    QMenu menu(this);
    QAction const* upAct = menu.addAction(tr("Monter"));
    QAction const* downAct = menu.addAction(tr("Descendre"));
    menu.addSeparator();
    QAction const* mergeDownAct = menu.addAction(tr("Merge Down"));
    menu.addSeparator();
    QAction* renameAct = menu.addAction(tr("Renommer"));
    QAction* resizeAct = menu.addAction(tr("Redimensionner calque"));
    QAction* deleteAct = menu.addAction(tr("Supprimer"));
    QAction const* dupAct = menu.addAction(tr("Dupliquer"));

    // --- Submenu Transformations ---
    QAction* rotateAct = nullptr;
    QMenu* transformMenu = menu.addMenu(tr("Transformations"));
    rotateAct = transformMenu->addAction(tr("Rotation"));
    rotateAct->setObjectName("rotateLayerAct");

    const bool isBottomLayer = (idx == 0);
    const bool isLockedLayer = layer ? layer->locked() : false;
    renameAct->setEnabled(!isBottomLayer && !isLockedLayer);
    deleteAct->setEnabled(!isBottomLayer && !isLockedLayer);
    resizeAct->setEnabled(!isBottomLayer && !isLockedLayer);

    const QAction* act = menu.exec(m_layersList->mapToGlobal(pos));
    if (!act)
        return;

    if (act == renameAct)
    {
        bool ok = false;
        const QString current = QString::fromStdString(layer ? layer->name() : "");
        const QString text = QInputDialog::getText(this, tr("Renommer le calque"), tr("Nom:"),
                                                   QLineEdit::Normal, current, &ok);
        if (ok)
            app().setLayerName(idx, text.toStdString());
    }
    else if (act == resizeAct)
    {
        if (!layer || !layer->image())
            return;

        const int curW = layer->image()->width();
        const int curH = layer->image()->height();
        if (curW <= 0 || curH <= 0)
            return;

        QDialog dlg(this);
        dlg.setWindowTitle(tr("Redimensionner le calque"));

        auto* v = new QVBoxLayout(&dlg);

        // Row: W / H
        auto* row = new QHBoxLayout();
        auto* wLabel = new QLabel(tr("Largeur:"), &dlg);
        auto* wSpin = new QSpinBox(&dlg);
        wSpin->setRange(1, 20000);
        wSpin->setValue(curW);

        auto* hLabel = new QLabel(tr("Hauteur:"), &dlg);
        auto* hSpin = new QSpinBox(&dlg);
        hSpin->setRange(1, 20000);
        hSpin->setValue(curH);

        row->addWidget(wLabel);
        row->addWidget(wSpin);
        row->addSpacing(10);
        row->addWidget(hLabel);
        row->addWidget(hSpin);
        v->addLayout(row);

        // Row: keep ratio
        auto* keepRatio = new QCheckBox(tr("Garder proportions 🔗"), &dlg);
        keepRatio->setChecked(true);
        v->addWidget(keepRatio);

        // Buttons
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        v->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        // Ratio logic
        const double ratio = static_cast<double>(curW) / static_cast<double>(curH);
        bool guard = false;

        auto syncHFromW = [&]()
        {
            if (!keepRatio->isChecked() || guard)
                return;
            guard = true;
            const int w = wSpin->value();
            const int h = std::max(1, static_cast<int>(std::lround(w / ratio)));
            hSpin->setValue(h);
            guard = false;
        };

        auto syncWFromH = [&]()
        {
            if (!keepRatio->isChecked() || guard)
                return;
            guard = true;
            const int h = hSpin->value();
            const int w = std::max(1, static_cast<int>(std::lround(h * ratio)));
            wSpin->setValue(w);
            guard = false;
        };

        connect(wSpin, qOverload<int>(&QSpinBox::valueChanged), &dlg, [&](int) { syncHFromW(); });
        connect(hSpin, qOverload<int>(&QSpinBox::valueChanged), &dlg, [&](int) { syncWFromH(); });
        connect(keepRatio, &QCheckBox::toggled, &dlg,
                [&](bool on)
                {
                    if (on)
                        syncHFromW();
                });

        if (dlg.exec() != QDialog::Accepted)
            return;

        const int newW = wSpin->value();
        const int newH = hSpin->value();

        app().resizeLayer(idx, newW, newH);
    }
    else if (act == deleteAct)
    {
        const QString layerName = QString::fromStdString(layer ? layer->name() : "");
        const int ret = QMessageBox::question(this, tr("Supprimer le calque"),
                                              tr("Supprimer le calque %1 ?").arg(layerName),
                                              QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes)
            app().removeLayer(idx);
    }
    else if (act == mergeDownAct)
        app().mergeLayerDown(idx);
    else if (act == upAct)
        moveLayerUp();
    else if (act == downAct)
        moveLayerDown();
    else if (act == dupAct)
    {
        if (idx != 0)
            app().duplicateLayer(idx);
    }
    else if (act == rotateAct)
    {
        showRotateLayerPopup(idx, m_layersList->mapToGlobal(pos));
    }
}
// GCOVR_EXCL_STOP

void MainWindow::onMergeDown()
{
    if (!m_layersList || !app().hasDocument())
        return;

    QListWidgetItem* item = m_layersList->currentItem();
    if (!item)
        return;

    const auto layerId = static_cast<std::uint64_t>(item->data(Qt::UserRole).toULongLong());
    const auto idxOpt = app::commands::findLayerIndexById(app().document(), layerId);
    if (!idxOpt.has_value() || idxOpt.value() == 0)
        return;
    const std::size_t idx = *idxOpt;

    auto layer = app().document().layerAt(idx);
    if (!layer || layer->locked())
        return;
    app().mergeLayerDown(idx);
}

void MainWindow::syncStrokeToolState()
{
    const bool pencilOn = m_pencilAct && m_pencilAct->isChecked();
    const bool eraserOn = m_eraseAct && m_eraseAct->isChecked();
    if (!canvas_)
        return;

    canvas_->setPencilEnable(pencilOn);
    canvas_->setEraserEnable(eraserOn);
}

void MainWindow::refreshUIAfterDocChange()
{
    if (m_dragLayerActive)
        return;

    const bool hasDoc = app().hasDocument();

    if (m_zoomInAct)
        m_zoomInAct->setEnabled(hasDoc);
    if (m_zoomOutAct)
        m_zoomOutAct->setEnabled(hasDoc);
    if (m_resetZoomAct)
        m_resetZoomAct->setEnabled(hasDoc);
    if (m_zoom05Act)
        m_zoom05Act->setEnabled(hasDoc);
    if (m_zoom1Act)
        m_zoom1Act->setEnabled(hasDoc);
    if (m_zoom2Act)
        m_zoom2Act->setEnabled(hasDoc);

    bool editable = false;
    if (hasDoc)
    {
        auto layer = app().document().layerAt(app().activeLayer());
        editable = (layer && !layer->locked() && layer->image());
    }
    if (m_bucketAct)
        m_bucketAct->setEnabled(hasDoc && editable);
    if (m_pickAct)
        m_pickAct->setEnabled(hasDoc && editable);
    if (m_selectToggleAct)
        m_selectToggleAct->setEnabled(hasDoc && editable);
    if (m_clearSelectionAct)
        m_clearSelectionAct->setEnabled(hasDoc && editable);
    if (m_colorPickerAct)
        m_colorPickerAct->setEnabled(hasDoc && editable);
    if (m_moveLayerAct)
        m_moveLayerAct->setEnabled(hasDoc && editable);
    if (m_pencilAct)
        m_pencilAct->setEnabled(hasDoc && editable);
    if (m_eraseAct)
        m_eraseAct->setEnabled(hasDoc && editable);

    if (!hasDoc)
    {
        if (canvas_)
        {
            canvas_->setMoveLayerEnable(false);
            canvas_->setPencilEnable(false);
            canvas_->clearDragLayerPreview();
        }
        if (m_pencilAct)
            syncStrokeToolState();
        if (m_pencilDock)
            m_pencilDock->setVisible(false);
        if (m_eraseDock)
            m_eraseDock->setVisible(false);
        if (m_moveLayerAct)
            m_moveLayerAct->setChecked(false);
        if (m_bucketAct)
            m_bucketAct->setChecked(false);
        if (m_pickAct)
            m_pickAct->setChecked(false);
        if (m_selectToggleAct)
            m_selectToggleAct->setChecked(false);
        if (m_eraseAct)
            syncStrokeToolState();

        clearUiStateOnClose();
        return;
    }

    if (canvas_)
        canvas_->setImage(Renderer::render(app().document()));

    {
        QSignalBlocker blocker(m_layersList);
        populateLayersList();

        const auto& sel = app().document().selection();
        canvas_->setSelectionRectOverlay(sel.hasMask() ? std::optional(sel.boundingRect())
                                                       : std::nullopt);

        if (m_pendingSelectLayerId_)
        {
            selectLayerInListById(*m_pendingSelectLayerId_);
            m_pendingSelectLayerId_.reset();
        }
        else
        {
            const auto idx = app().activeLayer();
            auto layer = app().document().layerAt(idx);
            if (layer)
                selectLayerInListById(layer->id());
        }
    }
    if (canvas_ && m_moveLayerAct)
        canvas_->setMoveLayerEnable(m_moveLayerAct->isChecked());

    updateLayerHeaderButtonsEnabled();
    updateLayerOverlayFromSelection();

    if (m_undoAct)
        m_undoAct->setEnabled(app().canUndo());
    if (m_redoAct)
        m_redoAct->setEnabled(app().canRedo());
}

void MainWindow::updateLayerOverlayFromSelection()
{
    if (!canvas_ || !app().hasDocument())
    {
        if (canvas_)
            canvas_->setLayerRectOverlay(std::nullopt);
        return;
    }

    auto idxOpt = currentLayerIndexFromSelection();
    if (!idxOpt.has_value())
    {
        canvas_->setLayerRectOverlay(std::nullopt);
        return;
    }

    const std::size_t idx = *idxOpt;
    if (idx == 0)  // background: pas d’overlay
    {
        canvas_->setLayerRectOverlay(std::nullopt);
        return;
    }

    auto layer = app().document().layerAt(idx);
    if (!layer || !layer->image())
    {
        canvas_->setLayerRectOverlay(std::nullopt);
        return;
    }

    common::Rect r;
    r.x = layer->offsetX();
    r.y = layer->offsetY();
    r.w = layer->image()->width();
    r.h = layer->image()->height();
    canvas_->setLayerRectOverlay(r);
}

void MainWindow::clearUiStateOnClose()
{
    m_currentFileName.clear();
    if (canvas_)
    {
        canvas_->clear();
        canvas_->setSelectionEnable(false);
        canvas_->setMoveLayerEnable(false);
        canvas_->setPencilEnable(false);
    }
    if (m_layersList)
        m_layersList->clear();
    if (m_bucketAct)
        m_bucketAct->setChecked(false);
    if (m_pickAct)
        m_pickAct->setChecked(false);
    if (m_selectToggleAct)
        m_selectToggleAct->setChecked(false);
    if (m_moveLayerAct)
        m_moveLayerAct->setChecked(false);
    if (m_pencilAct)
        m_pencilAct->setChecked(false);
    if (m_eraseAct)
        m_eraseAct->setChecked(false);

    m_bucketMode = false;
    m_pickMode = false;
    m_moveLayerMode = false;
}

void MainWindow::createActions()
{
    /* Menu Fichier */
    m_newAct = new QAction(tr("&Nouveau..."), this);
    m_newAct->setShortcut(QKeySequence::New);
    m_newAct->setStatusTip(tr("Créer une nouvelle image"));
    m_newAct->setObjectName("newAct");
    connect(m_newAct, &QAction::triggered, this, &MainWindow::newImage);

    m_openAct = new QAction(tr("&Ouvrir..."), this);
    m_openAct->setShortcut(QKeySequence::Open);
    m_openAct->setStatusTip(tr("Ouvrir une image existante"));
    m_openAct->setObjectName("openAct");
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openImage);

    m_saveAct = new QAction(tr("&Enregistrer sous..."), this);
    m_saveAct->setShortcut(QKeySequence::SaveAs);
    m_saveAct->setStatusTip(tr("Enregistrer l'image sous un nouveau nom"));
    m_saveAct->setObjectName("saveAct");
    connect(m_saveAct, &QAction::triggered, this, &MainWindow::saveImage);

    m_closeAct = new QAction(tr("&Fermer"), this);
    m_closeAct->setShortcut(QKeySequence::Close);
    m_closeAct->setStatusTip(tr("Fermer l'image actuelle"));
    connect(m_closeAct, &QAction::triggered, this, &MainWindow::closeImage);
    m_closeAct->setObjectName("act.close");

    m_exitAct = new QAction(tr("&Quitter"), this);
    m_exitAct->setShortcut(QKeySequence::Quit);
    m_exitAct->setStatusTip(tr("Quitter l'application"));
    m_exitAct->setObjectName("exitAct");
    connect(m_exitAct, &QAction::triggered, this, &QWidget::close);

    m_openEpgAct = new QAction(tr("Ouvrir un fichier &EPG..."), this);
    m_openEpgAct->setStatusTip(tr("Ouvrir un fichier EpiGimp (.epg)"));
    m_openEpgAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    m_openEpgAct->setObjectName("openEpgAct");
    connect(m_openEpgAct, &QAction::triggered, this, &MainWindow::openEpg);

    m_saveEpgAct = new QAction(tr("Enregistrer au format &EPG..."), this);
    m_saveEpgAct->setStatusTip(tr("Enregistrer l'image au format EpiGimp (.epg)"));
    m_saveEpgAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    m_saveEpgAct->setObjectName("saveEpgAct");
    connect(m_saveEpgAct, &QAction::triggered, this, &MainWindow::saveAsEpg);

    m_addLayerAct = new QAction(tr("Ajouter un calque"), this);
    m_addLayerAct->setStatusTip(tr("Ajouter un nouveau calque vide au document"));
    m_addLayerAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+N")));
    m_addLayerAct->setObjectName("addLayerAct");
    connect(m_addLayerAct, &QAction::triggered, this, &MainWindow::addNewLayer);

    m_addImageLayerAct = new QAction(tr("Ajouter une image en calque"), this);
    m_addImageLayerAct->setStatusTip(
        tr("Charger une image et l'ajouter en tant que nouveau calque"));
    m_addImageLayerAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+O")));
    m_addImageLayerAct->setObjectName("addImageLayerAct");
    connect(m_addImageLayerAct, &QAction::triggered, this, &MainWindow::addImageAsLayer);

    /* Undo / Redo actions */
    m_undoAct = new QAction(tr("&Annuler"), this);
    m_undoAct->setShortcut(QKeySequence::Undo);
    m_undoAct->setStatusTip(tr("Annuler la dernière action"));
    m_undoAct->setIcon(QIcon(":/icons/undo.svg"));
    m_undoAct->setEnabled(false);
    m_undoAct->setObjectName("undoAct");
    connect(m_undoAct, &QAction::triggered, this, [this]() { app().undo(); });

    m_redoAct = new QAction(tr("&Rétablir"), this);
    m_redoAct->setShortcut(QKeySequence::Redo);
    m_redoAct->setStatusTip(tr("Rétablir la dernière action annulée"));
    m_redoAct->setIcon(QIcon(":/icons/redo.svg"));
    m_redoAct->setEnabled(false);
    m_redoAct->setObjectName("redoAct");
    connect(m_redoAct, &QAction::triggered, this, [this]() { app().redo(); });

    /* Actions available on layers */

    m_layerUpAct = new QAction(tr("Monter le calque"), this);
    m_layerUpAct->setStatusTip(tr("Déplacer le calque vers le haut (au-dessus)"));
    m_layerUpAct->setShortcut(QKeySequence("Ctrl+]"));
    m_layerUpAct->setObjectName("layerUpAct");
    connect(m_layerUpAct, &QAction::triggered, this, &MainWindow::moveLayerUp);

    m_layerDownAct = new QAction(tr("Descendre le calque"), this);
    m_layerDownAct->setStatusTip(tr("Déplacer le calque vers le bas (en dessous)"));
    m_layerDownAct->setShortcut(QKeySequence("Ctrl+["));
    m_layerDownAct->setObjectName("layerDownAct");
    connect(m_layerDownAct, &QAction::triggered, this, &MainWindow::moveLayerDown);

    m_moveLayerAct = new QAction(tr("Déplacer le calque"), this);
    m_moveLayerAct->setCheckable(true);
    m_moveLayerAct->setChecked(false);
    m_moveLayerAct->setIcon(QIcon(":/icons/mouse.svg"));
    m_moveLayerAct->setShortcut(QKeySequence("Ctrl+M"));
    m_moveLayerAct->setStatusTip("Permet de déplacer le calque sélectionné");
    m_moveLayerAct->setObjectName("act.moveLayer");
    connect(
        m_moveLayerAct, &QAction::toggled, this,
        [this](bool on)
        {
            m_moveLayerMode = on;

            m_moveLayerAct->setIcon(QIcon(on ? ":/icons/mouse_select.svg" : ":/icons/mouse.svg"));
            if (canvas_)
                canvas_->setMoveLayerEnable(on);
            if (on)
            {
                if (m_bucketAct)
                    m_bucketAct->setChecked(false);
                if (m_pickAct)
                    m_pickAct->setChecked(false);
                if (m_selectToggleAct)
                    m_selectToggleAct->setChecked(false);

                m_bucketMode = false;
                m_pickMode = false;

                if (canvas_)
                    canvas_->setSelectionEnable(false);
            }
        });

    /* Color Picker */
    m_pickAct = new QAction(tr("Pipette"), this);
    m_pickAct->setCheckable(true);
    m_pickAct->setIcon(QIcon(":/icons/color_picker.svg"));
    m_pickAct->setShortcut(QKeySequence("O"));
    connect(m_pickAct, &QAction::toggled, this,
            [this](bool on)
            {
                m_pickMode = on;
                if (on)
                {
                    if (m_bucketAct)
                        m_bucketAct->setChecked(false);
                    if (m_selectToggleAct)
                        m_selectToggleAct->setChecked(false);
                    m_bucketMode = false;
                    if (canvas_)
                        canvas_->setSelectionEnable(false);
                    if (m_moveLayerAct)
                        m_moveLayerAct->setChecked(false);
                    m_pickAct->setIcon(QIcon(":/icons/color_picker_selec.svg"));
                }
                else
                {
                    m_pickAct->setIcon(QIcon(":/icons/color_picker.svg"));
                }
            });
    m_pickAct->setObjectName("act.picker");

    m_colorPickerAct = new QAction(tr("Couleur de remplissage"), this);
    m_colorPickerAct->setStatusTip(tr("Choisir la couleur utilisée par le pot de peinture"));
    m_colorPickerAct->setIcon(QIcon(":/icons/color_picker.svg"));
    m_colorPickerAct->setShortcut(QKeySequence(QStringLiteral("C")));
    m_colorPickerAct->setObjectName("colorPickerAct");
    connect(m_colorPickerAct, &QAction::triggered, this,
            [this]()
            {
                QColor chosen = QColorDialog::getColor(m_toolColor, this, tr("Choisir la couleur"));
                if (chosen.isValid())
                {
                    m_toolColor = chosen;
                    updateColorPickerIcon();
                    if (canvas_)
                        canvas_->setPencilColor(m_toolColor);
                }
            });

    m_pencilAct = new QAction(tr("Pinceau"), this);
    m_pencilAct->setCheckable(true);
    m_pencilAct->setShortcut(QKeySequence("P"));
    m_pencilAct->setIcon(QIcon(":/icons/pencil.svg"));
    connect(m_pencilAct, &QAction::toggled, this,
            [this](bool on)
            {
                if (canvas_)
                {
                    syncStrokeToolState();
                    canvas_->setSelectionEnable(false);
                }
                if (m_pencilDock)
                    m_pencilDock->setVisible(on);
                if (on)
                {
                    if (m_moveLayerAct)
                        m_moveLayerAct->setChecked(false);
                    if (m_bucketAct)
                        m_bucketAct->setChecked(false);
                    if (m_pickAct)
                        m_pickAct->setChecked(false);
                    if (m_selectToggleAct)
                        m_selectToggleAct->setChecked(false);
                    if (m_eraseAct)
                        m_eraseAct->setChecked(false);
                    if (m_eraseDock)
                        m_eraseDock->setVisible(false);
                    m_bucketMode = false;
                    m_pickMode = false;
                }
            });
    m_colorPickerAct->setObjectName("act.colorPick");

    /* Zoom actions */
    m_zoomInAct = new QAction(tr("Zoom &Avant"), this);
    m_zoomInAct->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAct->setStatusTip(tr("Agrandir"));
    m_zoomInAct->setObjectName("zoomInAct");
    connect(m_zoomInAct, &QAction::triggered, this, &MainWindow::zoomIn);

    m_zoomOutAct = new QAction(tr("Zoom A&rrière"), this);
    m_zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAct->setStatusTip(tr("Rétrécir"));
    m_zoomOutAct->setObjectName("zoomOutAct");
    connect(m_zoomOutAct, &QAction::triggered, this, &MainWindow::zoomOut);

    m_resetZoomAct = new QAction(tr("&Taille réelle"), this);
    m_resetZoomAct->setShortcut(QKeySequence("Ctrl+0"));
    m_resetZoomAct->setStatusTip(tr("Zoom 100%"));
    m_resetZoomAct->setObjectName("resetZoomAct");
    connect(m_resetZoomAct, &QAction::triggered, this, &MainWindow::resetZoom);

    m_zoom05Act = new QAction(tr("Zoom ×0.5"), this);
    m_zoom05Act->setShortcut(QKeySequence("Ctrl+1"));
    m_zoom05Act->setObjectName("Zoom05Act");
    connect(m_zoom05Act, &QAction::triggered, this,
            [this]()
            {
                if (canvas_)
                    canvas_->setScale(0.5);
            });

    m_zoom1Act = new QAction(tr("Zoom ×1"), this);
    m_zoom1Act->setShortcut(QKeySequence("Ctrl+2"));
    m_zoom1Act->setObjectName("zoom1Act");
    connect(m_zoom1Act, &QAction::triggered, this,
            [this]()
            {
                if (canvas_)
                    canvas_->setScale(1.0);
            });

    m_zoom2Act = new QAction(tr("Zoom ×2"), this);
    m_zoom2Act->setShortcut(QKeySequence("Ctrl+3"));
    m_zoom2Act->setObjectName("zoom2Act");
    connect(m_zoom2Act, &QAction::triggered, this,
            [this]()
            {
                if (canvas_)
                    canvas_->setScale(2.0);
            });

    /* Tools */
    m_selectToggleAct = new QAction(tr("Mode sélection"), this);
    m_selectToggleAct->setCheckable(true);
    connect(m_selectToggleAct, &QAction::toggled, this, &MainWindow::toggleSelectionMode);
    m_selectToggleAct->setObjectName("act.select");
    m_selectToggleAct->setShortcut(QKeySequence(QStringLiteral("S")));  // mode sélection

    m_clearSelectionAct = new QAction(tr("Effacer sélection"), this);
    connect(m_clearSelectionAct, &QAction::triggered, this, &MainWindow::clearSelection);
    m_clearSelectionAct->setObjectName("act.clearSelection");
    m_clearSelectionAct->setShortcut(QKeySequence(
        Qt::Key_Escape));  // tu as déjà escClearAct, mais c'est mieux si l’action porte aussi le raccourci

    m_bucketAct = new QAction(tr("Pot de peinture"), this);
    m_bucketAct->setCheckable(true);
    m_bucketAct->setShortcut(QKeySequence("Shift+B"));
    connect(m_bucketAct, &QAction::toggled, this,
            [this](bool on)
            {
                m_bucketMode = on;
                if (on && m_selectToggleAct)
                {
                    m_selectToggleAct->setChecked(false);
                    m_bucketAct->setIcon(QIcon(":/icons/bucket_selec.svg"));
                    if (m_moveLayerAct)
                        m_moveLayerAct->setChecked(false);
                }
                else
                    m_bucketAct->setIcon(QIcon(":/icons/bucket.svg"));
            });
    m_bucketAct->setObjectName("act.bucket");

    m_eraseAct = new QAction(tr("Gomme"), this);
    m_eraseAct->setCheckable(true);
    m_eraseAct->setShortcut(QKeySequence("Maj+E"));
    m_eraseAct->setStatusTip(tr("Effacer"));
    m_eraseAct->setIcon(QIcon(":/icons/eraser.svg"));
    connect(m_eraseAct, &QAction::toggled, this,
            [this](bool on)
            {
                if (canvas_)
                {
                    syncStrokeToolState();
                    canvas_->setSelectionEnable(false);
                    canvas_->setEraserEnable(on);
                }
                if (m_eraseDock)
                    m_eraseDock->setVisible(on);
                if (on)
                {
                    if (m_moveLayerAct)
                        m_moveLayerAct->setChecked(false);
                    if (m_bucketAct)
                        m_bucketAct->setChecked(false);
                    if (m_pickAct)
                        m_pickAct->setChecked(false);
                    if (m_selectToggleAct)
                        m_selectToggleAct->setChecked(false);
                    if (m_pencilAct)
                        m_pencilAct->setChecked(false);
                    if (m_pencilDock)
                        m_pencilDock->setVisible(false);
                    m_bucketMode = false;
                    m_pickMode = false;
                }
            });
    m_eraseAct->setObjectName("act.erase");

    m_shortcutsHelpAct = new QAction(tr("Raccourcis clavier"), this);
    m_shortcutsHelpAct->setShortcuts(
        {QKeySequence(QStringLiteral("Ctrl+/")), QKeySequence(QStringLiteral("Ctrl+Shift+/")),
         QKeySequence(QStringLiteral("Ctrl+?")), QKeySequence(Qt::Key_F1)});
    m_shortcutsHelpAct->setShortcutContext(Qt::ApplicationShortcut);
    m_shortcutsHelpAct->setStatusTip(tr("Afficher la liste des raccourcis clavier"));
    connect(m_shortcutsHelpAct, &QAction::triggered, this, &MainWindow::showShortcutHelpDialog);
    m_shortcutsHelpAct->setObjectName("act.shortcutsHelp");

    // Merge down layer
    m_layerMergeDownAct = new QAction(tr("Fusionner vers le bas"), this);
    m_layerMergeDownAct->setShortcut(
        QKeySequence(QStringLiteral("Ctrl+E")));  // classique "Merge Down" type Photoshop
    m_layerMergeDownAct->setShortcutContext(Qt::ApplicationShortcut);
    m_layerMergeDownAct->setObjectName("layerMergeDownAct");
    connect(m_layerMergeDownAct, &QAction::triggered, this, &MainWindow::onMergeDown);
    addAction(m_layerMergeDownAct);

    // Delete layer
    m_layerDeleteAct = new QAction(tr("Supprimer le calque"), this);
    m_layerDeleteAct->setShortcut(QKeySequence(Qt::Key_Delete));
    m_layerDeleteAct->setShortcutContext(Qt::ApplicationShortcut);
    m_layerDeleteAct->setObjectName("layerDeleteAct");
    connect(m_layerDeleteAct, &QAction::triggered, this,
            [this]()
            {
                if (!app().hasDocument())
                    return;
                auto idxOpt = currentLayerIndexFromSelection();
                if (!idxOpt || *idxOpt == 0)
                    return;
                auto layer = app().document().layerAt(*idxOpt);
                if (!layer || layer->locked())
                    return;
                app().removeLayer(*idxOpt);
            });
    addAction(m_layerDeleteAct);

    // Rename layer
    m_layerRenameAct = new QAction(tr("Renommer le calque"), this);
    m_layerRenameAct->setShortcut(QKeySequence(Qt::Key_F2));
    m_layerRenameAct->setShortcutContext(Qt::ApplicationShortcut);
    m_layerRenameAct->setObjectName("layerRenameAct");
    connect(m_layerRenameAct, &QAction::triggered, this,
            [this]()
            {
                if (!m_layersList)
                    return;
                auto* item = m_layersList->currentItem();
                if (!item)
                    return;
                onLayerDoubleClicked(item);
            });
    addAction(m_layerRenameAct);

    // Toggle lock layer
    m_layerToggleLockAct = new QAction(tr("Verrouiller / Déverrouiller"), this);
    m_layerToggleLockAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    m_layerToggleLockAct->setShortcutContext(Qt::ApplicationShortcut);
    m_layerToggleLockAct->setObjectName("layerToggleLockAct");
    connect(m_layerToggleLockAct, &QAction::triggered, this,
            [this]()
            {
                if (!app().hasDocument())
                    return;
                auto idxOpt = currentLayerIndexFromSelection();
                if (!idxOpt || *idxOpt == 0)
                    return;
                auto layer = app().document().layerAt(*idxOpt);
                if (!layer)
                    return;
                app().setLayerLocked(*idxOpt, !layer->locked());
            });
    addAction(m_layerToggleLockAct);

    // Toggle visibility layer
    m_layerToggleVisibleAct = new QAction(tr("Afficher / Masquer le calque"), this);
    m_layerToggleVisibleAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+H")));
    m_layerToggleVisibleAct->setShortcutContext(Qt::ApplicationShortcut);
    m_layerToggleVisibleAct->setObjectName("layerToggleVisibleAct");
    connect(m_layerToggleVisibleAct, &QAction::triggered, this,
            [this]()
            {
                if (!app().hasDocument())
                    return;
                auto idxOpt = currentLayerIndexFromSelection();
                if (!idxOpt)
                    return;
                auto layer = app().document().layerAt(*idxOpt);
                if (!layer)
                    return;
                app().setLayerVisible(*idxOpt, !layer->visible());
            });

    m_layerDuplicateAct = new QAction(tr("Dupliquer le calque"), this);
    m_layerDuplicateAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+J")));
    m_layerDuplicateAct->setShortcutContext(Qt::ApplicationShortcut);
    m_layerDuplicateAct->setObjectName("layerDuplicateAct");
    connect(m_layerDuplicateAct, &QAction::triggered, this,
            [this]()
            {
                if (!app().hasDocument())
                    return;
                auto idxOpt = currentLayerIndexFromSelection();
                if (!idxOpt)
                    return;
                auto layer = app().document().layerAt(*idxOpt);
                if (!layer || !layer->image())
                    return;
                app().duplicateLayer(*idxOpt);
            });
    addAction(m_layerDuplicateAct);

    m_focusCanvasAct = new QAction(tr("Focus Canvas"), this);
    m_focusCanvasAct->setShortcut(QKeySequence(Qt::Key_F6));
    m_focusCanvasAct->setShortcutContext(Qt::ApplicationShortcut);
    m_focusCanvasAct->setObjectName("focusCanvasAct");
    connect(m_focusCanvasAct, &QAction::triggered, this,
            [this]()
            {
                if (canvas_)
                    canvas_->setFocus();
            });
    addAction(m_focusCanvasAct);

    m_focusLayersAct = new QAction(tr("Focus Calques"), this);
    m_focusLayersAct->setShortcut(QKeySequence(Qt::Key_F7));
    m_focusLayersAct->setShortcutContext(Qt::ApplicationShortcut);
    m_focusLayersAct->setObjectName("focusLayersAct");
    connect(m_focusLayersAct, &QAction::triggered, this,
            [this]()
            {
                if (m_layersList)
                    m_layersList->setFocus();
            });
    addAction(m_focusLayersAct);

    addAction(m_layerToggleVisibleAct);
    auto* escClearAct = new QAction(this);
    escClearAct->setShortcut(QKeySequence(Qt::Key_Escape));
    escClearAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(escClearAct, &QAction::triggered, this,
            [this]()
            {
                if (!app().hasDocument())
                    return;
                app().clearSelectionRect();
                if (canvas_)
                    canvas_->clearSelectionRect();
            });
    addAction(escClearAct);
    addAction(m_shortcutsHelpAct);
}

void MainWindow::createMenus()
{
    /* Menu Fichier */
    m_fileMenu = menuBar()->addMenu(tr("&Fichier"));
    m_fileMenu->addAction(m_newAct);
    m_fileMenu->addAction(m_openAct);
    m_fileMenu->addAction(m_addLayerAct);
    m_fileMenu->addAction(m_addImageLayerAct);
    m_fileMenu->addAction(m_openEpgAct);
    m_fileMenu->addAction(m_saveAct);
    m_fileMenu->addAction(m_saveEpgAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_closeAct);
    m_fileMenu->addAction(m_exitAct);

    /* View Menu */
    m_viewMenu = menuBar()->addMenu(tr("&Vue"));
    m_viewMenu->addAction(m_zoomInAct);
    m_viewMenu->addAction(m_zoomOutAct);
    m_viewMenu->addAction(m_resetZoomAct);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_zoom05Act);
    m_viewMenu->addAction(m_zoom1Act);
    m_viewMenu->addAction(m_zoom2Act);

    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_layerUpAct);
    m_viewMenu->addAction(m_layerDownAct);

    /* Tool menu */
    m_cmdMenu = menuBar()->addMenu(tr("&Outils"));
    m_cmdMenu->addAction(m_moveLayerAct);
    m_cmdMenu->addAction(m_selectToggleAct);
    m_cmdMenu->addAction(m_clearSelectionAct);
    m_cmdMenu->addSeparator();
    m_cmdMenu->addAction(m_bucketAct);
    m_cmdMenu->addAction(m_colorPickerAct);

    m_helpMenu = menuBar()->addMenu(tr("&Aide"));
    if (m_shortcutsHelpAct)
        m_helpMenu->addAction(m_shortcutsHelpAct);

    // Undo/Redo
    m_cmdMenu->addSeparator();
    if (m_undoAct)
        m_cmdMenu->addAction(m_undoAct);
    if (m_redoAct)
        m_cmdMenu->addAction(m_redoAct);
}

void MainWindow::createLayersPanel()
{
    m_layersDock = new QDockWidget(tr("Calques"), this);

    auto* root = new QWidget(m_layersDock);
    auto* v = new QVBoxLayout(root);
    v->setContentsMargins(6, 6, 6, 6);
    v->setSpacing(6);

    // --- header bar ---
    auto* header = new QWidget(root);
    auto* h = new QHBoxLayout(header);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(4);

    m_layerAddBtn = new QToolButton(header);
    m_layerAddBtn->setIcon(QIcon(":/icons/add_layer.svg"));
    m_layerAddBtn->setToolTip(tr("Ajouter un calque"));
    m_layerAddBtn->setObjectName("btn.layer.add");

    m_layerDeleteBtn = new QToolButton(header);
    m_layerDeleteBtn->setIcon(QIcon(":/icons/remove_layer.svg"));
    m_layerDeleteBtn->setToolTip(tr("Supprimer le calque"));
    m_layerDeleteBtn->setObjectName("btn.layer.del");

    m_layerUpBtn = new QToolButton(header);
    m_layerUpBtn->setIcon(QIcon(":/icons/layer_up.svg"));
    m_layerUpBtn->setToolTip(tr("Monter"));
    m_layerUpBtn->setObjectName("btn.layer.up");

    m_layerDownBtn = new QToolButton(header);
    m_layerDownBtn->setIcon(QIcon(":/icons/layer_down.svg"));
    m_layerDownBtn->setToolTip(tr("Descendre"));
    m_layerDownBtn->setObjectName("btn.layer.down");

    m_layerMergeDownBtn = new QToolButton(header);
    m_layerMergeDownBtn->setIcon(QIcon(":/icons/merge_layer_down.svg"));
    m_layerMergeDownBtn->setToolTip(tr("Fusionner vers le bas"));
    m_layerMergeDownBtn->setObjectName("btn.layer.mergeDown");

    h->addWidget(m_layerAddBtn);
    h->addWidget(m_layerDeleteBtn);
    h->addSpacing(8);
    h->addWidget(m_layerUpBtn);
    h->addWidget(m_layerDownBtn);
    h->addWidget(m_layerMergeDownBtn);
    h->addStretch();

    // --- list ---
    m_layersList = new QListWidget(root);
    m_layersList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_layersList->setDragDropMode(QAbstractItemView::NoDragDrop);
    m_layersList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_layersList->setObjectName("layerList");
    m_layersList->setFocusPolicy(Qt::StrongFocus);
    m_layersList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    v->addWidget(header);
    v->addWidget(m_layersList);

    m_layersDock->setWidget(root);
    m_layersDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable |
                              QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::RightDockWidgetArea, m_layersDock);

    // --- connections ---
    connect(m_layerAddBtn, &QToolButton::clicked, this, &MainWindow::addNewLayer);

    connect(m_layerUpBtn, &QToolButton::clicked, this, &MainWindow::moveLayerUp);
    connect(m_layerDownBtn, &QToolButton::clicked, this, &MainWindow::moveLayerDown);

    connect(m_layerMergeDownBtn, &QToolButton::clicked, this, &MainWindow::onMergeDown);

    connect(m_layerDeleteBtn, &QToolButton::clicked, this,
            [this]()
            {
                if (!app().hasDocument())
                    return;

                auto idxOpt = currentLayerIndexFromSelection();
                if (!idxOpt.has_value() || idxOpt.value() == 0)
                    return;

                const std::size_t idx = *idxOpt;

                auto layer = app().document().layerAt(idx);
                if (!layer || layer->locked())
                    return;

                const QString layerName = QString::fromStdString(layer->name());
                if (QMessageBox::question(this, tr("Supprimer le calque"),
                                          tr("Supprimer le calque %1 ?").arg(layerName),
                                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
                {
                    app().removeLayer(idx);
                }
            });

    connect(m_layersList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem* current, QListWidgetItem*)
            {
                if (!current || !app().hasDocument())
                    return;

                const auto layerId =
                    static_cast<std::uint64_t>(current->data(Qt::UserRole).toULongLong());
                auto idx = app::commands::findLayerIndexById(app().document(), layerId);
                if (idx.has_value())
                    app().setActiveLayer(*idx);

                updateLayerHeaderButtonsEnabled();
                updateLayerOverlayFromSelection();
            });

    connect(m_layersList, &QListWidget::itemDoubleClicked, this, &MainWindow::onLayerDoubleClicked);
    connect(m_layersList, &QListWidget::customContextMenuRequested, this,
            &MainWindow::onShowLayerContextMenu);

    updateLayerHeaderButtonsEnabled();
}

void MainWindow::createToolBar()
{
    m_toolsTb = addToolBar(tr("Outils"));
    m_toolsTb->setMovable(false);
    m_toolsTb->setFloatable(false);
    m_toolsTb->setIconSize(QSize(22, 22));

    m_toolsGroup = new QActionGroup(this);
    m_toolsGroup->setExclusive(true);

    // --- Move Layer (checkable tool, part of exclusive group)
    if (m_moveLayerAct)
    {
        m_moveLayerAct->setCheckable(true);  // au cas où
        m_toolsGroup->addAction(m_moveLayerAct);
        m_toolsTb->addAction(m_moveLayerAct);
    }

    // --- Select tool (checkable tool, part of exclusive group)
    if (m_selectToggleAct)
    {
        m_selectToggleAct->setIcon(QIcon(":/icons/selection.svg"));
        m_selectToggleAct->setToolTip(tr("Sélection rectangulaire"));
        m_selectToggleAct->setCheckable(true);

        m_toolsGroup->addAction(m_selectToggleAct);
        m_toolsTb->addAction(m_selectToggleAct);
    }

    // --- Clear selection (not a tool)
    if (m_clearSelectionAct)
    {
        m_clearSelectionAct->setIcon(QIcon(":/icons/clear_selection.svg"));
        m_clearSelectionAct->setToolTip(tr("Effacer la sélection"));
        m_toolsTb->addAction(m_clearSelectionAct);
    }

    m_toolsTb->addSeparator();

    // --- Bucket tool (checkable tool, part of exclusive group)
    if (m_bucketAct)
    {
        m_bucketAct->setIcon(QIcon(":/icons/bucket.svg"));
        m_bucketAct->setToolTip(tr("Pot de peinture"));
        m_bucketAct->setCheckable(true);

        m_toolsGroup->addAction(m_bucketAct);
        m_toolsTb->addAction(m_bucketAct);
    }

    // --- Fill color picker (not a tool)
    if (m_colorPickerAct)
    {
        m_colorPickerAct->setToolTip(tr("Couleur de remplissage"));
        m_toolsTb->addAction(m_colorPickerAct);
    }

    // --- Pipette tool (checkable tool, part of exclusive group)
    if (m_pickAct)
    {
        m_pickAct->setCheckable(true);
        m_toolsGroup->addAction(m_pickAct);
        m_toolsTb->addAction(m_pickAct);
    }

    if (m_pencilAct)
    {
        m_pencilAct->setToolTip(tr("Pinceau"));
        m_toolsGroup->addAction(m_pencilAct);
        m_toolsTb->addAction(m_pencilAct);
    }

    if (m_eraseAct)
    {
        m_toolsGroup->addAction(m_eraseAct);
        m_toolsTb->addAction(m_eraseAct);
    }

    m_toolsTb->addSeparator();

    if (m_undoAct)
        m_toolsTb->addAction(m_undoAct);
    if (m_redoAct)
        m_toolsTb->addAction(m_redoAct);

    // Pencil properties dock
    m_pencilDock = new QDockWidget(tr("Pinceau"), this);
    auto* pencilWidget = new QWidget(m_pencilDock);
    auto* bv = new QHBoxLayout(pencilWidget);
    bv->setContentsMargins(6, 6, 6, 6);

    auto* sizeLblPen = new QLabel(tr("Taille"), pencilWidget);
    m_pencilSizeSpin = new QSpinBox(pencilWidget);
    m_pencilSizeSpin->setRange(1, 200);
    m_pencilSizeSpin->setValue(8);

    auto* opacityLblPen = new QLabel(tr("Opacity"), pencilWidget);
    m_pencilOpacitySpin = new QSpinBox(pencilWidget);
    m_pencilOpacitySpin->setRange(1, 100);
    m_pencilOpacitySpin->setValue(100);

    bv->addWidget(sizeLblPen);
    bv->addWidget(m_pencilSizeSpin);
    bv->addWidget(opacityLblPen);
    bv->addWidget(m_pencilOpacitySpin);

    m_eraseDock = new QDockWidget(tr("Gomme"), this);
    auto* eraseWidget = new QWidget(m_eraseDock);
    auto* layout = new QVBoxLayout(eraseWidget);
    layout->setContentsMargins(6, 6, 6, 6);

    auto* sizeLblEr = new QLabel(tr("Taille"), eraseWidget);
    m_eraseSizeSpin = new QSpinBox(eraseWidget);
    m_eraseSizeSpin->setRange(1, 200);
    m_eraseSizeSpin->setValue(8);

    auto* opacityLblEr = new QLabel(tr("Opacity"), eraseWidget);
    m_eraseOpacitySpin = new QSpinBox(eraseWidget);
    m_eraseOpacitySpin->setRange(1, 100);
    m_eraseOpacitySpin->setValue(100);

    layout->addWidget(sizeLblEr);
    layout->addWidget(m_eraseSizeSpin);
    layout->addWidget(opacityLblEr);
    layout->addWidget(m_eraseOpacitySpin);
    layout->addSpacing(12);

    if (canvas_)
    {
        // connect pencil size and opacity to canvas preview

        connect(m_pencilSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this,
                [this](const int v)
                {
                    if (canvas_)
                        canvas_->setPencilSize(v);
                });
        connect(m_pencilOpacitySpin, qOverload<int>(&QSpinBox::valueChanged), this,
                [this](const double v)
                {
                    if (canvas_)
                        canvas_->setPencilOpacity(v / 100.0);
                });
        canvas_->setPencilSize(m_pencilSizeSpin->value());
        canvas_->setPencilOpacity(m_pencilOpacitySpin->value() / 100.0);

        // connect eraser size and opacity to canvas preview

        connect(m_eraseSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this,
                [this](const int v)
                {
                    if (canvas_)
                        canvas_->setEraserSize(v);
                });
        connect(m_eraseOpacitySpin, qOverload<int>(&QSpinBox::valueChanged), this,
                [this](double v)
                {
                    if (canvas_)
                        canvas_->setEraserOpacity(v / 100.0);
                });
        // initialize canvas size
        canvas_->setEraserSize(m_eraseSizeSpin->value());
        canvas_->setEraserOpacity(m_eraseOpacitySpin->value() / 100.0);
    }

    pencilWidget->setLayout(bv);
    m_pencilDock->setWidget(pencilWidget);
    addDockWidget(Qt::TopDockWidgetArea, m_pencilDock);
    m_pencilDock->setVisible(false);

    eraseWidget->setLayout(layout);
    m_eraseDock->setWidget(eraseWidget);
    addDockWidget(Qt::TopDockWidgetArea, m_eraseDock);
    m_eraseDock->setVisible(false);
}

void MainWindow::setDirty(bool on)
{
    m_dirty = on;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    QString base =
        m_currentFileName.isEmpty() ? tr("Sans titre") : QFileInfo(m_currentFileName).fileName();

    if (m_dirty)
        base += " *";
    setWindowTitle(tr("%1 - EpiGimp 2.0").arg(base));
}

bool MainWindow::confirmDiscardIfDirty(const QString& actionLabel, bool epg)
{
    if (!app().hasDocument() || !app().isDirty())
        return true;

    const auto ret = QMessageBox::warning(
        this, tr("Modifications non enregistrées"),
        tr("Le document a des modifications non enregistrées.\n\n%1 sans enregistrer ?")
            .arg(actionLabel),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

    if (ret == QMessageBox::Cancel)
        return false;

    if (ret == QMessageBox::Save)
    {
        if (epg)
        {
            saveAsEpg();
        }
        else
        {
            saveImage();
        }
        return !m_dirty;
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!confirmDiscardIfDirty(tr("Quitter"), true))
    {
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::populateLayersList()
{
    if (!m_layersList)
        return;

    m_layersList->clear();

    if (!app().hasDocument())
        return;

    const auto& doc = app().document();
    const auto count = doc.layerCount();
    if (count == 0)
        return;

    for (std::size_t i = count; i-- > 0;)
    {
        auto layer = app().document().layerAt(i);
        if (!layer)
            continue;

        auto* item = new QListWidgetItem(m_layersList);

        const auto layerId = static_cast<qulonglong>(layer->id());
        item->setData(Qt::UserRole, layerId);
        const bool isBottom = (i == 0);
        const bool isLocked = layer->locked();

        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        item->setFlags(flags);
        m_layersList->addItem(item);

        // Create a custom widget for the layer row
        auto* row = new QWidget();
        row->setFixedHeight(56);
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(4, 2, 4, 2);

        auto* eyeBtn = new QPushButton(row);
        eyeBtn->setFlat(true);
        eyeBtn->setIcon(QIcon(layer->visible() ? ":/icons/eye.svg" : ":/icons/eye_closed.svg"));
        eyeBtn->setIconSize(QSize(16, 16));
        eyeBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        eyeBtn->setFixedSize(28, 28);
        eyeBtn->setToolTip(tr("Changer la visibilité"));
        eyeBtn->setEnabled(true);

        auto* lockBtn = new QPushButton(row);
        lockBtn->setFlat(true);
        lockBtn->setIcon(QIcon(layer->locked() ? ":/icons/lock.svg" : ":/icons/unlock.svg"));
        lockBtn->setIconSize(QSize(16, 16));
        lockBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        lockBtn->setFixedSize(28, 28);
        lockBtn->setToolTip(tr("Verrouiller/déverrouiller"));
        lockBtn->setEnabled(!isBottom);

        auto* thumb = new QLabel(row);
        thumb->setFixedSize(48, 48);
        thumb->setAlignment(Qt::AlignCenter);
        thumb->setPixmap(createLayerThumbnail(layer, thumb->size()));
        thumb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        auto* nameLbl = new QLabel(row);
        nameLbl->setText(QString::fromStdString(layer->name()));
        nameLbl->setMinimumWidth(0);
        nameLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        nameLbl->setWordWrap(false);

        auto* opacitySpin = new QSpinBox(row);
        opacitySpin->setRange(0, 100);
        opacitySpin->setValue(static_cast<int>(layer->opacity() * 100.0f));
        opacitySpin->setFixedWidth(55);
        opacitySpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        opacitySpin->setEnabled(!isLocked);

        h->addWidget(eyeBtn);
        h->addWidget(lockBtn);
        h->addWidget(nameLbl, 1);
        h->addWidget(opacitySpin);
        h->addWidget(thumb);

        m_layersList->setItemWidget(item, row);
        item->setSizeHint(QSize(0, 56));

        connect(eyeBtn, &QPushButton::clicked, this,
                [this, layerId]()
                {
                    const auto idx = app::commands::findLayerIndexById(app().document(), layerId);
                    if (!idx.has_value())
                        return;
                    auto layer = app().document().layerAt(*idx);
                    if (!layer)
                        return;
                    app().setLayerVisible(*idx, !layer->visible());
                });

        connect(lockBtn, &QPushButton::clicked, this,
                [this, layerId, isBottom]()
                {
                    if (isBottom)
                        return;
                    const auto idx = app::commands::findLayerIndexById(app().document(), layerId);
                    if (!idx.has_value())
                        return;
                    auto layer = app().document().layerAt(*idx);
                    if (!layer)
                        return;
                    app().setLayerLocked(*idx, !layer->locked());
                });

        connect(opacitySpin, &QSpinBox::valueChanged, this,
                [this, layerId](int a)
                {
                    const auto idx = app::commands::findLayerIndexById(app().document(), layerId);
                    if (!idx.has_value())
                        return;
                    auto layer = app().document().layerAt(*idx);
                    if (!layer)
                        return;
                    app().setLayerOpacity(*idx, static_cast<float>(a) / 100.F);
                });
    }
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
        return out;
    }

    const ImageBuffer& lb = *layer->image();
    if (lb.width() <= 0 || lb.height() <= 0)
        return out;

    QImage img = ImageConversion::imageBufferToQImage(lb, QImage::Format_ARGB32_Premultiplied);

    QSize target = size;
    QSize scaledSize = img.size();
    if (img.width() > target.width() || img.height() > target.height())
        scaledSize = img.size().scaled(target, Qt::KeepAspectRatio);

    QImage scaled = img.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPixmap canvas(target);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    const int cx = (target.width() - scaled.width()) / 2;
    const int cy = (target.height() - scaled.height()) / 2;
    painter.drawImage(cx, cy, scaled);
    painter.end();

    if (!layer->visible())
    {
        QImage dim = canvas.toImage();
        QPainter p(&dim);
        p.fillRect(dim.rect(), QColor(0, 0, 0, 120));
        return QPixmap::fromImage(dim);
    }

    return canvas;
}

std::optional<std::size_t> MainWindow::currentLayerIndexFromSelection() const
{
    if (!m_layersList || !app().hasDocument())
        return std::nullopt;

    auto* cur = m_layersList->currentItem();
    if (!cur)
        return std::nullopt;

    const auto layerId = static_cast<std::uint64_t>(cur->data(Qt::UserRole).toULongLong());
    return app::commands::findLayerIndexById(app().document(), layerId);
}

void MainWindow::updateLayerHeaderButtonsEnabled()
{
    const bool hasDoc = app().hasDocument();
    if (!hasDoc)
    {
        if (m_layerDeleteBtn)
            m_layerDeleteBtn->setEnabled(false);
        if (m_layerUpBtn)
            m_layerUpBtn->setEnabled(false);
        if (m_layerDownBtn)
            m_layerDownBtn->setEnabled(false);
        if (m_layerMergeDownBtn)
            m_layerMergeDownBtn->setEnabled(false);
        if (m_layerAddBtn)
            m_layerAddBtn->setEnabled(false);
        return;
    }

    if (m_layerAddBtn)
        m_layerAddBtn->setEnabled(true);

    auto idxOpt = currentLayerIndexFromSelection();
    if (!idxOpt.has_value())
    {
        if (m_layerDeleteBtn)
            m_layerDeleteBtn->setEnabled(false);
        if (m_layerUpBtn)
            m_layerUpBtn->setEnabled(false);
        if (m_layerDownBtn)
            m_layerDownBtn->setEnabled(false);
        if (m_layerMergeDownBtn)
            m_layerMergeDownBtn->setEnabled(false);
        return;
    }

    const std::size_t idx = *idxOpt;
    const auto n = app().document().layerCount();
    auto layer = app().document().layerAt(idx);

    const bool isBottom = (idx == 0);
    const bool isLocked = (layer && layer->locked());

    if (m_layerDeleteBtn)
        m_layerDeleteBtn->setEnabled(!isBottom && !isLocked);
    if (m_layerMergeDownBtn)
        m_layerMergeDownBtn->setEnabled(!isBottom && !isLocked);

    if (m_layerUpBtn)
        m_layerUpBtn->setEnabled(idx + 1 < n);
    if (m_layerDownBtn)
        m_layerDownBtn->setEnabled(idx > 0);
}

void MainWindow::selectLayerInListById(std::uint64_t id)
{
    if (!m_layersList)
        return;

    for (int row = 0; row < m_layersList->count(); ++row)
    {
        auto* it = m_layersList->item(row);
        if (!it)
            continue;
        const auto itemId = static_cast<std::uint64_t>(it->data(Qt::UserRole).toULongLong());
        if (itemId == id)
        {
            m_layersList->setCurrentItem(it);
            return;
        }
    }
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
    p.setBrush(QBrush(m_toolColor));
    p.setPen(Qt::black);
    p.drawRect(0, 0, sz - 1, sz - 1);
    p.end();
    m_colorPickerAct->setIcon(QIcon(pix));
}

QString MainWindow::formatActionShortcut(const QAction* a) const
{
    if (!a)
        return {};
    QString t = a->text();
    t.remove('&');

    const auto scs = a->shortcuts();
    if (scs.isEmpty())
        return t;

    QStringList parts;
    for (const auto& sc : scs)
        parts << sc.toString(QKeySequence::NativeText);

    return QStringLiteral("%1\t%2").arg(t, parts.join(QStringLiteral(" / ")));
}

void MainWindow::showRotateLayerPopup(std::size_t idx, const QPoint& globalPos)
{
    if (!app().hasDocument())
        return;

    auto layer = app().document().layerAt(idx);
    if (!layer || !layer->image())
        return;

    // règle : layer non lock, sauf background (idx=0 autorisé même lock)
    if (idx != 0 && layer->locked())
        return;

    closeRotateLayerPopup();

    m_rotateTargetIdx = idx;

    // Widget flottant style palette
    m_rotatePopup = new QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint);
    m_rotatePopup->setObjectName("popup.rotateLayer");
    m_rotatePopup->setAttribute(Qt::WA_DeleteOnClose, true);

    auto* layout = new QVBoxLayout(m_rotatePopup);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    auto* title = new QLabel(tr("Rotation"), m_rotatePopup);
    QFont f = title->font();
    f.setBold(true);
    title->setFont(f);
    layout->addWidget(title);

    m_rotateValueLbl = new QLabel(tr("0°"), m_rotatePopup);
    m_rotateValueLbl->setObjectName("lbl.rotate.deg");
    layout->addWidget(m_rotateValueLbl);

    // --- Angle input row (Spin + quick buttons) ---
    auto* rowAngle = new QHBoxLayout();

    m_rotateSpin = new QSpinBox(m_rotatePopup);
    m_rotateSpin->setObjectName("spin.rotate.deg");

    // On autorise signé pour permettre horaire/anti-horaire
    // Convention UI: + = horaire (on appliquera la conversion plus bas)
    m_rotateSpin->setRange(-360, 360);
    m_rotateSpin->setValue(0);
    m_rotateSpin->setSingleStep(1);
    m_rotateSpin->setSuffix(QStringLiteral("°"));

    auto* btnCCW90 = new QPushButton(QStringLiteral("↺ 90°"), m_rotatePopup);
    btnCCW90->setObjectName("btn.rotate.ccw90");

    auto* btnCW90 = new QPushButton(QStringLiteral("↻ 90°"), m_rotatePopup);
    btnCW90->setObjectName("btn.rotate.cw90");

    rowAngle->addWidget(m_rotateSpin, 1);
    rowAngle->addWidget(btnCCW90);
    rowAngle->addWidget(btnCW90);

    layout->addLayout(rowAngle);

    auto* row = new QHBoxLayout();
    auto* applyBtn = new QPushButton(tr("Appliquer"), m_rotatePopup);
    applyBtn->setObjectName("btn.rotate.apply");
    auto* closeBtn = new QPushButton(tr("Fermer"), m_rotatePopup);
    closeBtn->setObjectName("btn.rotate.close");
    row->addWidget(applyBtn);
    row->addWidget(closeBtn);
    layout->addLayout(row);

    // MAJ label + mémorise angle
    connect(m_rotateSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int v)
            {
                m_rotateLastDeg = v;
                if (m_rotateValueLbl)
                    m_rotateValueLbl->setText(QString::number(v) + QChar(0x00B0));
            });

    // Quick buttons
    connect(btnCW90, &QPushButton::clicked, this,
            [this]()
            {
                if (m_rotateSpin)
                    m_rotateSpin->setValue(m_rotateSpin->value() - 90);  // ↺ anti-horaire (dans UI)
            });
    connect(btnCCW90, &QPushButton::clicked, this,
            [this]()
            {
                if (m_rotateSpin)
                    m_rotateSpin->setValue(m_rotateSpin->value() + 90);  // ↻ horaire (dans UI)
            });

    // Apply => appel AppService
    connect(applyBtn, &QPushButton::clicked, this,
            [this]()
            {
                if (!m_rotateTargetIdx || !app().hasDocument())
                    return;
                try
                {
                    app().rotateLayer(*m_rotateTargetIdx, static_cast<float>(m_rotateLastDeg));
                }
                catch (const std::exception& e)
                {
                    statusBar()->showMessage(e.what(), 2000);
                }
            });

    // Close
    connect(closeBtn, &QPushButton::clicked, this, [this]() { closeRotateLayerPopup(); });

    m_rotatePopup->installEventFilter(this);
    m_rotatePopup->adjustSize();
    m_rotatePopup->move(globalPos + QPoint(8, 8));
    m_rotatePopup->show();
    m_rotatePopup->raise();
    m_rotatePopup->activateWindow();
}

void MainWindow::closeRotateLayerPopup()
{
    if (!m_rotatePopup)
        return;
    m_rotatePopup->close();
    m_rotatePopup->deleteLater();
    m_rotatePopup = nullptr;
    m_rotateSpin = nullptr;
    m_rotateValueLbl = nullptr;
    m_rotateTargetIdx.reset();
    m_rotateLastDeg = 0;
}