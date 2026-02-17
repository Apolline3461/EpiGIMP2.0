#pragma once

#include <QAction>
#include <QColor>
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

class MainWindow : public QMainWindow
{
    Q_OBJECT
    friend class ImageActions;

   public:
    explicit MainWindow(app::AppService& svc, QWidget* parent = nullptr);
    ~MainWindow() override {}

   private slots:
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

    CanvasWidget* canvas_{nullptr};

    void refreshUIAfterDocChange();
    void clearUiStateOnClose();

    void createActions();
    void createMenus();
    void createLayersPanel();
    void createToolBar();

    void populateLayersList();
    QPixmap createLayerThumbnail(const std::shared_ptr<class Layer>& layer,
                                 const QSize& size) const;

    // void keyPressEvent(QKeyEvent* event) override;
    // void keyReleaseEvent(QKeyEvent* event) override;

    void selectLayerInListById(std::uint64_t id);
    void updateColorPickerIcon();

    // Membres internes
    QString m_currentFileName;

    QMenu* m_fileMenu{nullptr};
    QMenu* m_viewMenu{nullptr};
    QMenu* m_cmdMenu{nullptr};

    QToolBar* m_toolsTb = nullptr;
    QActionGroup* m_toolsGroup = nullptr;

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
    QAction* m_bucketAct{nullptr};
    QAction* m_colorPickerAct{nullptr};
    QAction* m_pickAct{nullptr};
    bool m_bucketMode{false};
    bool m_pickMode{false};
    QColor m_bucketColor{Qt::black};

    bool m_handMode{false};
    bool m_panningActive{false};
    QPoint m_lastPanPos;

    QAction* m_addLayerAct{nullptr};
    QAction* m_addImageLayerAct{nullptr};
    QDockWidget* m_layersDock{nullptr};
    QListWidget* m_layersList{nullptr};
    QAction* m_layerUpAct{nullptr};
    QAction* m_layerDownAct{nullptr};

    // Document and layers UI
    std::optional<std::uint64_t> m_pendingSelectLayerId_;

    // Historique des commandes (undo/redo)
    QAction* m_undoAct{nullptr};
    QAction* m_redoAct{nullptr};
};
