#include "ui/window.hpp"

#include <QActionGroup>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDateTime>
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
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextEdit>
#include <QTextOption>
#include <QToolBar>
#include <QVBoxLayout>

#include "app/commands/CommandUtils.hpp"
#include "common/Geometry.hpp"
#include "core/Document.hpp"
#include "core/Layer.hpp"
#include "io/EpgFormat.hpp"
#include "io/EpgJson.hpp"
#include "ui/CanvasWidget.hpp"
#include "ui/ImageConversion.hpp"
#include "ui/Render.hpp"

MainWindow::MainWindow(app::AppService& svc, QWidget* parent) : QMainWindow(parent), svc_(svc)
{
    canvas_ = new CanvasWidget(this);
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

                app().bucketFill(p, newColor);
            });
    connect(canvas_, &CanvasWidget::beginStroke, this,
            [this](common::Point p)
            {
                if (!m_pencilAct || !m_pencilAct->isChecked())
                    return;
                app::ToolParams params;
                params.tool = app::ToolKind::Pencil;
                params.size = (m_pencilSizeSpin) ? m_pencilSizeSpin->value() : 8;
                (void)0;  // hardness removed
                params.color = (static_cast<uint32_t>(m_toolColor.red()) << 24) |
                               (static_cast<uint32_t>(m_toolColor.green()) << 16) |
                               (static_cast<uint32_t>(m_toolColor.blue()) << 8) |
                               static_cast<uint32_t>(m_toolColor.alpha());
                app().beginStroke(params, p);
            });

    connect(canvas_, &CanvasWidget::moveStroke, this,
            [this](common::Point p)
            {
                if (!m_pencilAct || !m_pencilAct->isChecked())
                    return;
                app().moveStroke(p);
            });

    connect(canvas_, &CanvasWidget::endStroke, this,
            [this]()
            {
                if (!m_pencilAct || !m_pencilAct->isChecked())
                    return;
                app().endStroke();
            });

    app().documentChanged.connect([this]() { refreshUIAfterDocChange(); });

    refreshUIAfterDocChange();
}

void MainWindow::refreshUIAfterDocChange()
{
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

    if (!hasDoc)
    {
        clearUiStateOnClose();
        return;
    }
    canvas_->setImage(Renderer::render(app().document()));

    populateLayersList();

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

    if (m_undoAct)
        m_undoAct->setEnabled(app().canUndo());
    if (m_redoAct)
        m_redoAct->setEnabled(app().canRedo());
}

void MainWindow::clearUiStateOnClose()
{
    m_currentFileName.clear();
    if (canvas_)
        canvas_->clear();
    if (m_layersList)
        m_layersList->clear();

    // TODO: reset bucket/selection tool state in UI (actions checked etc.)
}

void MainWindow::createActions()
{
    /* Menu Fichier */
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

    m_addLayerAct = new QAction(tr("Ajouter un calque"), this);
    m_addLayerAct->setStatusTip(tr("Ajouter un nouveau calque vide au document"));
    connect(m_addLayerAct, &QAction::triggered, this, &MainWindow::addNewLayer);

    m_addImageLayerAct = new QAction(tr("Ajouter une image en calque"), this);
    m_addImageLayerAct->setStatusTip(
        tr("Charger une image et l'ajouter en tant que nouveau calque"));
    connect(m_addImageLayerAct, &QAction::triggered, this, &MainWindow::addImageAsLayer);

    /* Undo / Redo actions */
    m_undoAct = new QAction(tr("&Annuler"), this);
    m_undoAct->setShortcut(QKeySequence::Undo);
    m_undoAct->setStatusTip(tr("Annuler la dernière action"));
    m_undoAct->setIcon(QIcon(":/icons/undo.svg"));
    m_undoAct->setEnabled(false);
    connect(m_undoAct, &QAction::triggered, this, [this]() { app().undo(); });

    m_redoAct = new QAction(tr("&Rétablir"), this);
    m_redoAct->setShortcut(QKeySequence::Redo);
    m_redoAct->setStatusTip(tr("Rétablir la dernière action annulée"));
    m_redoAct->setIcon(QIcon(":/icons/redo.svg"));
    m_redoAct->setEnabled(false);
    connect(m_redoAct, &QAction::triggered, this, [this]() { app().redo(); });

    /* Actions available on layers */

    m_layerUpAct = new QAction(tr("Monter le calque"), this);
    m_layerUpAct->setStatusTip(tr("Déplacer le calque vers le haut (au-dessus)"));
    m_layerUpAct->setShortcut(QKeySequence("Ctrl+]"));
    connect(m_layerUpAct, &QAction::triggered, this, &MainWindow::moveLayerUp);

    m_layerDownAct = new QAction(tr("Descendre le calque"), this);
    m_layerDownAct->setStatusTip(tr("Déplacer le calque vers le bas (en dessous)"));
    m_layerDownAct->setShortcut(QKeySequence("Ctrl+["));
    connect(m_layerDownAct, &QAction::triggered, this, &MainWindow::moveLayerDown);

    // TODO: m_layerMergeDown

    /* Color Picker */
    m_pickAct = new QAction(tr("Pipette"), this);
    m_pickAct->setCheckable(true);
    m_pickAct->setIcon(QIcon(":/icons/color_picker.svg"));

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
                    m_pickAct->setIcon(QIcon(":/icons/color_picker_selec.svg"));
                }
                else
                {
                    m_pickAct->setIcon(QIcon(":/icons/color_picker.svg"));
                }
            });
    m_colorPickerAct = new QAction(tr("Couleur de remplissage"), this);
    m_colorPickerAct->setStatusTip(tr("Choisir la couleur utilisée par le pot de peinture"));
    m_colorPickerAct->setIcon(QIcon(":/icons/color_picker.svg"));
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
    m_pencilAct->setIcon(QIcon(":/icons/pencil.svg"));
    connect(m_pencilAct, &QAction::toggled, this,
            [this](bool on)
            {
                if (on)
                {
                    if (m_bucketAct)
                        m_bucketAct->setChecked(false);
                    if (m_selectToggleAct)
                        m_selectToggleAct->setChecked(false);
                    if (canvas_)
                        canvas_->setSelectionEnable(false);
                }
            });

    /* Zoom actions */
    m_zoomInAct = new QAction(tr("Zoom &Avant"), this);
    m_zoomInAct->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAct->setStatusTip(tr("Agrandir"));
    connect(m_zoomInAct, &QAction::triggered, this, &MainWindow::zoomIn);

    m_zoomOutAct = new QAction(tr("Zoom A&rrière"), this);
    m_zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAct->setStatusTip(tr("Rétrécir"));
    connect(m_zoomOutAct, &QAction::triggered, this, &MainWindow::zoomOut);

    m_resetZoomAct = new QAction(tr("&Taille réelle"), this);
    m_resetZoomAct->setShortcut(QKeySequence("Ctrl+0"));
    m_resetZoomAct->setStatusTip(tr("Zoom 100%"));
    connect(m_resetZoomAct, &QAction::triggered, this, &MainWindow::resetZoom);

    m_zoom05Act = new QAction(tr("Zoom ×0.5"), this);
    m_zoom05Act->setShortcut(QKeySequence("Ctrl+1"));
    connect(m_zoom05Act, &QAction::triggered, this,
            [this]()
            {
                if (canvas_)
                    canvas_->setScale(0.5);
            });
    m_zoom1Act = new QAction(tr("Zoom ×1"), this);
    m_zoom1Act->setShortcut(QKeySequence("Ctrl+2"));
    connect(m_zoom1Act, &QAction::triggered, this,
            [this]()
            {
                if (canvas_)
                    canvas_->setScale(1.0);
            });
    m_zoom2Act = new QAction(tr("Zoom ×2"), this);
    m_zoom2Act->setShortcut(QKeySequence("Ctrl+3"));
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

    m_clearSelectionAct = new QAction(tr("Effacer sélection"), this);
    connect(m_clearSelectionAct, &QAction::triggered, this, &MainWindow::clearSelection);

    m_bucketAct = new QAction(tr("Pot de peinture"), this);
    m_bucketAct->setCheckable(true);
    connect(m_bucketAct, &QAction::toggled, this,
            [this](bool on)
            {
                m_bucketMode = on;
                if (on && m_selectToggleAct)
                {
                    m_selectToggleAct->setChecked(false);
                    m_bucketAct->setIcon(QIcon(":/icons/bucket_selec.svg"));
                }
                else
                    m_bucketAct->setIcon(QIcon(":/icons/bucket.svg"));
            });
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
    m_cmdMenu->addAction(m_selectToggleAct);
    m_cmdMenu->addAction(m_clearSelectionAct);
    m_cmdMenu->addSeparator();
    m_cmdMenu->addAction(m_bucketAct);
    m_cmdMenu->addAction(m_colorPickerAct);

    // Undo/Redo
    m_cmdMenu->addSeparator();
    if (m_undoAct)
        m_cmdMenu->addAction(m_undoAct);
    if (m_redoAct)
        m_cmdMenu->addAction(m_redoAct);
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
    spec.color = common::colors::Transparent;

    const int defW = app().document().width();
    const int defH = app().document().height();

    QDialog dlg(
        this);  // TODO: support layer size (w/h) in AppService::addLayer (resize layer buffer or add "LayerSpec size")
    dlg.setWindowTitle(tr("Ajouter un calque"));
    QVBoxLayout* dlgLayout = new QVBoxLayout(&dlg);

    QHBoxLayout* sizeLayout = new QHBoxLayout();
    QLabel* wLabel = new QLabel(tr("Largeur:"), &dlg);
    QSpinBox* wSpin = new QSpinBox(&dlg);
    wSpin->setRange(1, 10000);
    wSpin->setValue(defW);
    QLabel* hLabel = new QLabel(tr("Hauteur:"), &dlg);
    QSpinBox* hSpin = new QSpinBox(&dlg);
    hSpin->setRange(1, 10000);
    hSpin->setValue(defH);
    sizeLayout->addWidget(wLabel);
    sizeLayout->addWidget(wSpin);
    sizeLayout->addSpacing(8);
    sizeLayout->addWidget(hLabel);
    sizeLayout->addWidget(hSpin);
    dlgLayout->addLayout(sizeLayout);

    QHBoxLayout* colorLayout = new QHBoxLayout();
    QCheckBox* fillCheck = new QCheckBox(tr("Remplir avec une couleur"), &dlg);
    QPushButton* colorBtn = new QPushButton(tr("Choisir la couleur"), &dlg);
    QLabel* colorPreview = new QLabel(&dlg);
    colorPreview->setFixedSize(24, 24);
    colorPreview->setFrameStyle(QFrame::Box | QFrame::Plain);
    colorPreview->setAutoFillBackground(true);
    QColor chosenColor = QColor(255, 255, 255, 255);
    auto updatePreview = [&]()
    {
        QPalette pal = colorPreview->palette();
        pal.setColor(QPalette::Window, chosenColor);
        colorPreview->setPalette(pal);
    };
    updatePreview();
    connect(colorBtn, &QPushButton::clicked, &dlg,
            [&]()
            {
                QColor c = QColorDialog::getColor(chosenColor, &dlg, tr("Choisir la couleur"));
                if (c.isValid())
                {
                    chosenColor = c;
                    updatePreview();
                }
            });
    colorLayout->addWidget(fillCheck);
    colorLayout->addWidget(colorBtn);
    colorLayout->addWidget(colorPreview);
    dlgLayout->addLayout(colorLayout);

    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addStretch();
    QPushButton* okBtn = new QPushButton(tr("OK"), &dlg);
    QPushButton* cancelBtn = new QPushButton(tr("Annuler"), &dlg);
    buttons->addWidget(okBtn);
    buttons->addWidget(cancelBtn);
    dlgLayout->addLayout(buttons);
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    int w = wSpin->value();
    int h = hSpin->value();
    const bool fillWithColor = fillCheck->isChecked();

    if (w <= 0 || h <= 0)
        return;

    if (fillWithColor)
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

void MainWindow::createLayersPanel()
{
    m_layersDock = new QDockWidget(tr("Calques"), this);
    m_layersList = new QListWidget(m_layersDock);
    m_layersList->setSelectionMode(QAbstractItemView::SingleSelection);

    m_layersList->setDragDropMode(QAbstractItemView::NoDragDrop);  //disable for the moment

    m_layersList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_layersList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem* current, QListWidgetItem*)
            {
                if (!current || !app().hasDocument())
                    return;

                const auto layerId =
                    static_cast<std::uint64_t>(current->data(Qt::UserRole).toULongLong());
                std::optional<std::size_t> idx =
                    app::commands::findLayerIndexById(app().document(), layerId);
                if (idx.has_value())
                    app().setActiveLayer(*idx);
            });

    connect(m_layersList, &QListWidget::itemDoubleClicked, this, &MainWindow::onLayerDoubleClicked);
    connect(m_layersList, &QListWidget::customContextMenuRequested, this,
            &MainWindow::onShowLayerContextMenu);

    m_layersDock->setWidget(m_layersList);
    addDockWidget(Qt::RightDockWidgetArea, m_layersDock);
}

void MainWindow::createToolBar()
{
    m_toolsTb = addToolBar(tr("Outils"));
    m_toolsTb->setMovable(false);
    m_toolsTb->setFloatable(false);
    m_toolsTb->setIconSize(QSize(22, 22));

    m_toolsGroup = new QActionGroup(this);
    m_toolsGroup->setExclusive(true);

    if (m_selectToggleAct)
    {
        m_selectToggleAct->setIcon(QIcon(":/icons/selection.svg"));
        m_selectToggleAct->setToolTip(tr("Sélection rectangulaire"));
        m_selectToggleAct->setCheckable(true);
        m_toolsGroup->addAction(m_selectToggleAct);
        m_toolsTb->addAction(m_selectToggleAct);
    }
    if (m_clearSelectionAct)
    {
        m_clearSelectionAct->setIcon(QIcon(":/icons/clear_selection.svg"));
        m_clearSelectionAct->setToolTip(tr("Effacer la sélection"));
        m_toolsTb->addAction(m_clearSelectionAct);
    }

    m_toolsTb->addSeparator();

    if (m_bucketAct)
    {
        m_bucketAct->setIcon(QIcon(":/icons/bucket.svg"));
        m_bucketAct->setToolTip(tr("Pot de peinture"));
        m_bucketAct->setCheckable(true);
        m_toolsGroup->addAction(m_bucketAct);
        m_toolsTb->addAction(m_bucketAct);
    }

    if (m_colorPickerAct)
    {
        m_colorPickerAct->setToolTip(tr(
            "Couleur de remplissage"));  // image de la pipette et ca modifie juste la couleur courante
        m_toolsTb->addAction(m_colorPickerAct);
    }

    if (m_pickAct)
    {
        m_toolsGroup->addAction(m_pickAct);
        m_toolsTb->addAction(m_pickAct);
    }
    if (m_pencilAct)
    {
        m_pencilAct->setToolTip(tr("Pinceau"));
        m_toolsGroup->addAction(m_pencilAct);
        m_toolsTb->addAction(m_pencilAct);
    }

    // Create a small popup menu for pencil sizes when clicking the pencil icon
    QMenu* pencilSizeMenu = new QMenu(this);
    const std::vector<int> sizes = {1, 2, 4, 8, 16};
    for (int s : sizes)
    {
        QAction* a = new QAction(QString::number(s), pencilSizeMenu);
        a->setData(s);
        pencilSizeMenu->addAction(a);
        connect(a, &QAction::triggered, this,
                [this, s]()
                {
                    if (m_pencilSizeSpin)
                        m_pencilSizeSpin->setValue(s);
                    if (m_pencilAct)
                        m_pencilAct->setChecked(true);
                });
    }
    pencilSizeMenu->addSeparator();
    QAction* otherAct = new QAction(tr("Autre…"), pencilSizeMenu);
    pencilSizeMenu->addAction(otherAct);
    connect(otherAct, &QAction::triggered, this,
            [this]()
            {
                bool ok = false;
                const int cur = (m_pencilSizeSpin) ? m_pencilSizeSpin->value() : 8;
                const int v = QInputDialog::getInt(this, tr("Taille personnalisée"),
                                                   tr("Taille (px):"), cur, 1, 2000, 1, &ok);
                if (ok)
                {
                    if (m_pencilSizeSpin)
                        m_pencilSizeSpin->setValue(v);
                    if (m_pencilAct)
                        m_pencilAct->setChecked(true);
                }
            });
    // Show menu when pencil action is triggered
    connect(m_pencilAct, &QAction::triggered, this,
            [this, pencilSizeMenu]()
            {
                if (!m_toolsTb || !m_pencilAct)
                {
                    pencilSizeMenu->popup(QCursor::pos());
                    return;
                }
                QWidget* w = m_toolsTb->widgetForAction(m_pencilAct);
                if (w)
                {
                    const QPoint pos = w->mapToGlobal(QPoint(0, w->height()));
                    pencilSizeMenu->popup(pos);
                }
                else
                {
                    pencilSizeMenu->popup(QCursor::pos());
                }
            });

    m_toolsTb->addSeparator();

    if (m_undoAct)
        m_toolsTb->addAction(m_undoAct);
    if (m_redoAct)
        m_toolsTb->addAction(m_redoAct);
    // Pencil properties dock
    m_pencilDock = new QDockWidget(tr("Pinceau"), this);
    QWidget* pencilWidget = new QWidget(m_pencilDock);
    QHBoxLayout* bv = new QHBoxLayout(pencilWidget);
    bv->setContentsMargins(6, 6, 6, 6);

    QLabel* sizeLbl = new QLabel(tr("Taille"), pencilWidget);
    m_pencilSizeSpin = new QSpinBox(pencilWidget);
    m_pencilSizeSpin->setRange(1, 200);
    m_pencilSizeSpin->setValue(8);
    bv->addWidget(sizeLbl);
    bv->addWidget(m_pencilSizeSpin);

    // connect pencil size to canvas preview
    if (m_pencilSizeSpin && canvas_)
    {
        connect(m_pencilSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this,
                [this](int v)
                {
                    if (canvas_)
                        canvas_->setPencilSize(v);
                });
        // initialize canvas size
        canvas_->setPencilSize(m_pencilSizeSpin->value());
    }

    pencilWidget->setLayout(bv);
    m_pencilDock->setWidget(pencilWidget);
    addDockWidget(Qt::TopDockWidgetArea, m_pencilDock);
    m_pencilDock->setVisible(false);
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

        auto* nameLbl = new QLabel(row);
        nameLbl->setText(QString::fromStdString(layer->name()));
        nameLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto* opacitySpin = new QSpinBox(row);
        opacitySpin->setRange(0, 100);
        opacitySpin->setValue(static_cast<int>(layer->opacity() * 100.0f));
        opacitySpin->setFixedWidth(55);
        opacitySpin->setEnabled(!isLocked);

        h->addWidget(eyeBtn);
        h->addWidget(lockBtn);
        h->addWidget(nameLbl);
        h->addWidget(opacitySpin);
        h->addWidget(thumb);

        m_layersList->setItemWidget(item, row);
        item->setSizeHint(row->sizeHint());

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
    QAction* upAct = menu.addAction(tr("Monter"));
    QAction* downAct = menu.addAction(tr("Descendre"));
    menu.addSeparator();
    QAction* mergeDownAct = menu.addAction(tr("Merge Down"));
    menu.addSeparator();
    QAction* renameAct = menu.addAction(tr("Renommer"));
    // QAction* resizeAct = menu.addAction(tr("Redimensionner..."));
    QAction* deleteAct = menu.addAction(tr("Supprimer"));

    const bool isBottomLayer = (idx == 0);
    const bool isLockedLayer = layer ? layer->locked() : false;
    renameAct->setEnabled(!isBottomLayer && !isLockedLayer);
    deleteAct->setEnabled(!isBottomLayer && !isLockedLayer);

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
    /*else if (act == resizeAct) // revoir ca plus tard
    {
        const int curW = app().document().width();
        const int curH = app().document().height();

        QDialog dlg(this);
        dlg.setWindowTitle(tr("Redimensionner le calque"));
        QVBoxLayout* dlgLayout = new QVBoxLayout(&dlg);

        QHBoxLayout* sizeLayout = new QHBoxLayout();
        QLabel* wLabel = new QLabel(tr("Largeur:"), &dlg);
        QSpinBox* wSpin = new QSpinBox(&dlg);
        wSpin->setRange(1, 10000);
        wSpin->setValue(curW);
        QLabel* hLabel = new QLabel(tr("Hauteur:"), &dlg);
        QSpinBox* hSpin = new QSpinBox(&dlg);
        hSpin->setRange(1, 10000);
        hSpin->setValue(curH);
        sizeLayout->addWidget(wLabel);
        sizeLayout->addWidget(wSpin);
        sizeLayout->addSpacing(8);
        sizeLayout->addWidget(hLabel);
        sizeLayout->addWidget(hSpin);
        dlgLayout->addLayout(sizeLayout);

        QHBoxLayout* buttons = new QHBoxLayout();
        buttons->addStretch();
        QPushButton* okBtn = new QPushButton(tr("OK"), &dlg);
        QPushButton* cancelBtn = new QPushButton(tr("Annuler"), &dlg);
        buttons->addWidget(okBtn);
        buttons->addWidget(cancelBtn);
        dlgLayout->addLayout(buttons);
        connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

        if (dlg.exec() != QDialog::Accepted)
            return;

        const int w = wSpin->value();
        const int h = hSpin->value();

        auto lyr = app().document().layerAt(*idx);
        if (lyr && lyr->image())
        {
            ImageBuffer newBuf(w, h);
            newBuf.fill(0u);
            const ImageBuffer& old = *lyr->image();
            const int copyW = std::min(w, old.width());
            const int copyH = std::min(h, old.height());
            for (int yy = 0; yy < copyH; ++yy)
            {
                for (int xx = 0; xx < copyW; ++xx)
                {
                    newBuf.setPixel(xx, yy, old.getPixel(xx, yy));
                }
            }
            lyr->setImageBuffer(std::make_shared<ImageBuffer>(newBuf));
        }
    } */
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

void MainWindow::onMergeDown()
{
    if (!m_layersList || !app().hasDocument())
        return;

    QListWidgetItem* item = m_layersList->currentItem();
    if (!item)
        return;

    const auto layerId = static_cast<std::uint64_t>(item->data(Qt::UserRole).toULongLong());
    const auto idxOpt = app::commands::findLayerIndexById(app().document(), layerId);
    if (!idxOpt || idxOpt == 0)
        return;
    const std::size_t idx = *idxOpt;

    auto layer = app().document().layerAt(idx);
    if (!layer || layer->locked())
        return;
    app().mergeLayerDown(idx);
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
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible de sauvegarder %1.\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(QString::fromLocal8Bit(e.what())));
    }
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
    if (canvas_)
        canvas_->clearSelectionRect();

    // TODO: render selection overlay from doc selection, not from CanvasWidget local state.
}

void MainWindow::toggleSelectionMode(bool enabled)
{
    if (canvas_)
        canvas_->setSelectionEnable(enabled);
    if (m_selectToggleAct)
        m_selectToggleAct->setChecked(enabled);

    // si on active la sélection, désactiver le pot de peinture
    if (enabled && m_bucketAct)
    {
        m_bucketAct->setChecked(false);
        m_bucketMode = false;
    }
}

void MainWindow::newImage()
{
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

    mainLayout->addLayout(wLay);
    mainLayout->addLayout(hLay);
    mainLayout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;

    m_currentFileName.clear();
    app().newDocument({wSpin->value(), hSpin->value()}, 72.f);
    setWindowTitle(tr("Sans titre - EpiGimp 2.0"));
    statusBar()->showMessage(
        tr("Nouvelle image créée: %1x%2").arg(wSpin->value()).arg(hSpin->value()), 2000);
}

void MainWindow::openImage()
{
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
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible d'exporter %1.\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(QString::fromLocal8Bit(e.what())));
    }
}

void MainWindow::closeImage()
{
    if (!app().hasDocument())
        return;
    app().closeDocument();
    setWindowTitle(tr("EpiGimp 2.0"));
    statusBar()->showMessage(tr("Image fermée"), 2000);
}

void MainWindow::openEpg()
{
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
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, tr("Erreur"),
                              tr("Impossible d'ouvrir %1.\n%2")
                                  .arg(QDir::toNativeSeparators(fileName))
                                  .arg(QString::fromLocal8Bit(e.what())));
    }
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
