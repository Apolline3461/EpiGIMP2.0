#pragma once

#include <QAction>
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
#include <QScrollArea>
#include <QString>

#include <memory>

#include "core/Document.hpp"

class ImageActions;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    friend class ImageActions;

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

   private slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

    void openEpg();
    void saveAsEpg();
    void addNewLayer();
    void addImageAsLayer();
    void onLayerItemChanged(QListWidgetItem* item);
    void onLayerDoubleClicked(QListWidgetItem* item);
    void onLayersRowsMoved(const QModelIndex& parent, int start, int end,
                           const QModelIndex& destination, int row);
    void onShowLayerContextMenu(const QPoint& pos);
    void onMergeDown();

   private:
    void createActions();
    void createMenus();
    void createLayersPanel();
    void updateImageDisplay();
    void scaleImage(double factor);
    void setScaleAndCenter(double newScale);

    void populateLayersList();
    void updateImageFromDocument();
    QPixmap createLayerThumbnail(const std::shared_ptr<class Layer>& layer,
                                 const QSize& size) const;

    // évènements et panning
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    // Membres internes
    QLabel* m_imageLabel;
    QScrollArea* m_scrollArea;
    QImage m_currentImage;
    QString m_currentFileName;
    double m_scaleFactor;

    QMenu* m_fileMenu;
    QMenu* m_viewMenu;

    QAction* m_newAct;
    QAction* m_openAct;
    QAction* m_saveAct;
    QAction* m_closeAct;
    QAction* m_exitAct;
    QAction* m_zoomInAct;
    QAction* m_zoomOutAct;
    QAction* m_resetZoomAct;
    QAction* m_zoom05Act;
    QAction* m_zoom1Act;
    QAction* m_zoom2Act;
    // panning state
    bool m_handMode{false};
    bool m_panningActive{false};
    QPoint m_lastPanPos;
    QAction* m_openEpgAct;
    QAction* m_saveEpgAct;
    QAction* m_addLayerAct{nullptr};
    QAction* m_addImageLayerAct{nullptr};

    // Document and layers UI
    std::unique_ptr<Document> m_document;
    QDockWidget* m_layersDock{nullptr};
    QListWidget* m_layersList{nullptr};
    // layer dragging state (drag the selected layer on the canvas)
    bool m_layerDragActive{false};
    int m_dragLayerIndex{-1};
    QPoint m_layerDragStartDocPos;  // in document pixel coords
    int m_layerDragInitialOffsetX{0};
    int m_layerDragInitialOffsetY{0};
};
