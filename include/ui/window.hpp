#pragma once

#include <QAction>
#include <QCloseEvent>
#include <QColor>
#include <QDial>
#include <QDockWidget>
#include <QEvent>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QModelIndex>
#include <QObject>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QScrollArea>
#include <QString>
#include <QToolButton>

#include <memory>
#include <optional>

#include "app/AppService.hpp"

// Définitions pour les analyseurs (clangd) qui ne connaissent pas la
// macro `slots`. Ne pas redéfinir pour `moc`.
#if !defined(Q_MOC_RUN)
#if !defined(slots)
#define slots
#endif
#endif

class ImageActions;
class ImageLabel;
class CanvasWidget;
class QToolBar;
class QActionGroup;
class QSpinBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    friend class ImageActions;

   public:
    explicit MainWindow(app::AppService& svc, QWidget* parent = nullptr);
    ~MainWindow() override {}

    // NOLINTNEXTLINE(unknownMacro)
   private slots:

    void showShortcutHelpDialog();

    void zoomIn();
    void zoomOut();
    void resetZoom();

    void clearSelection();
    void toggleSelectionMode(bool enabled);

    void newImage();
    void openImage();
    void saveImage();
    void closeImage();

    void openEpg();
    void saveAsEpg();
    void addNewLayer();
    void addImageAsLayer();
    void onLayerDoubleClicked(QListWidgetItem* item);

    void moveLayerUp();
    void moveLayerDown();
    void onShowLayerContextMenu(const QPoint& pos);
    void onMergeDown();

   private:
    app::AppService& svc_;
    app::AppService& app()
    {
        return svc_;
    }
    const app::AppService& app() const
    {
        return svc_;
    }
    void syncStrokeToolState();
    void refreshUIAfterDocChange();
    void updateLayerOverlayFromSelection();
    void clearUiStateOnClose();

    void createActions();
    void createMenus();
    void createLayersPanel();
    void createToolBar();

    void setDirty(bool on);
    void updateWindowTitle();
    bool confirmDiscardIfDirty(const QString& actionLabel, bool epg);

    void closeEvent(QCloseEvent* event) override;

    void populateLayersList();
    QPixmap createLayerThumbnail(const std::shared_ptr<class Layer>& layer,
                                 const QSize& size) const;
    std::optional<std::size_t> currentLayerIndexFromSelection() const;
    void updateLayerHeaderButtonsEnabled();

    void selectLayerInListById(std::uint64_t id);
    void updateColorPickerIcon();

    QString formatActionShortcut(const QAction* a) const;

    void showRotateLayerPopup(std::size_t idx, const QPoint& globalPos);
    void closeRotateLayerPopup();

    // Membres internes
    CanvasWidget* canvas_{nullptr};

    QString m_currentFileName;

    QMenu* m_fileMenu{nullptr};
    QMenu* m_viewMenu{nullptr};
    QMenu* m_cmdMenu{nullptr};
    QMenu* m_helpMenu{nullptr};

    QToolBar* m_toolsTb = nullptr;
    QToolButton* m_layerAddBtn = nullptr;
    QToolButton* m_layerDeleteBtn = nullptr;
    QToolButton* m_layerUpBtn = nullptr;
    QToolButton* m_layerDownBtn = nullptr;
    QToolButton* m_layerMergeDownBtn = nullptr;

    QActionGroup* m_toolsGroup = nullptr;

    QAction* m_shortcutsHelpAct{nullptr};

    QAction* m_newAct{nullptr};
    QAction* m_openAct{nullptr};
    QAction* m_saveAct{nullptr};
    QAction* m_closeAct{nullptr};
    QAction* m_exitAct{nullptr};
    QAction* m_zoomInAct{nullptr};
    QAction* m_zoomOutAct{nullptr};
    QAction* m_resetZoomAct{nullptr};
    QAction* m_zoom05Act{nullptr};
    QAction* m_zoom1Act{nullptr};
    QAction* m_zoom2Act{nullptr};

    QAction* m_openEpgAct{nullptr};
    QAction* m_saveEpgAct{nullptr};

    QAction* m_clearSelectionAct{nullptr};
    QAction* m_selectToggleAct{nullptr};
    QAction* m_lassoAct{nullptr};

    bool m_bucketMode{false};
    bool m_pickMode{false};
    QColor m_bucketColor{Qt::black};
    QAction* m_bucketAct{nullptr};
    QAction* m_colorPickerAct{nullptr};
    QAction* m_pickAct{nullptr};

    QAction* m_moveLayerAct{nullptr};
    QAction* m_layerMergeDownAct{nullptr};
    QAction* m_layerDeleteAct{nullptr};
    QAction* m_copySelectionAct{nullptr};
    QAction* m_layerRenameAct{nullptr};
    QAction* m_layerToggleLockAct{nullptr};
    QAction* m_layerToggleVisibleAct{nullptr};

    bool m_moveLayerMode{false};
    bool m_dragLayerActive{false};
    std::size_t m_dragLayerIdx{0};
    common::Point m_dragStartDoc{0, 0};
    common::Point m_dragStartOffset{0, 0};
    QImage m_dragBaseImage;   // rendu "base" pendant le drag (doc sans le layer déplacé)
    QImage m_dragLayerImage;  // image du layer déplacé (seul)

    QColor m_toolColor{Qt::black};

    QAction* m_pencilAct{nullptr};
    QDockWidget* m_pencilDock{nullptr};
    QSpinBox* m_pencilSizeSpin{nullptr};
    QSpinBox* m_pencilOpacitySpin{nullptr};

    QAction* m_eraseAct{nullptr};
    QDockWidget* m_eraseDock{nullptr};
    QSpinBox* m_eraseSizeSpin{nullptr};
    QSpinBox* m_eraseOpacitySpin{nullptr};

    bool m_handMode{false};
    bool m_panningActive{false};
    QPoint m_lastPanPos;

    QAction* m_addLayerAct{nullptr};
    QAction* m_addImageLayerAct{nullptr};
    QDockWidget* m_layersDock{nullptr};
    QListWidget* m_layersList{nullptr};
    QAction* m_layerUpAct{nullptr};
    QAction* m_layerDownAct{nullptr};
    QAction* m_layerDuplicateAct{nullptr};

    // tranform
    QWidget* m_rotatePopup{nullptr};
    QSpinBox* m_rotateSpin{nullptr};
    QLabel* m_rotateValueLbl{nullptr};
    std::optional<std::size_t> m_rotateTargetIdx{};
    int m_rotateLastDeg{0};

    bool m_dirty{false};

    // Document and layers UI
    std::optional<std::uint64_t> m_pendingSelectLayerId_;

    // Historique des commandes (undo/redo)
    QAction* m_undoAct{nullptr};
    QAction* m_redoAct{nullptr};

    // navigation focus
    QAction* m_focusCanvasAct{nullptr};
    QAction* m_focusLayersAct{nullptr};
};
