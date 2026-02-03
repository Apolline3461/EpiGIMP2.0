#pragma once

#include <QAction>
#include <QColor>
#include <QEvent>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QScrollArea>
#include <QString>

#include "app/History.hpp"

#include <core/Selection.hpp>

// Définitions pour les analyseurs (clangd) qui ne connaissent pas la
// macro `slots`. Ne pas redéfinir pour `moc`.
#if !defined(Q_MOC_RUN)
#if !defined(slots)
#define slots
#endif
#endif

class ImageActions;
class ImageLabel;

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

    void onMouseSelection(const QRect& rect);

    void clearSelection();
    void toggleSelectionMode(bool enabled);

    void openEpg();
    void saveAsEpg();

   private:
    void createActions();
    void createMenus();
    void updateImageDisplay();
    void scaleImage(double factor);
    void setScaleAndCenter(double newScale);

    // évènements et panning
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    // Membres internes
    ImageLabel* m_imageLabel;
    QScrollArea* m_scrollArea;
    QImage m_currentImage;
    QString m_currentFileName;
    double m_scaleFactor;

    QMenu* m_fileMenu;
    QMenu* m_viewMenu;
    QMenu* m_cmdMenu;

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
    QAction* m_clearSelectionAct;
    QAction* m_selectToggleAct;
    QAction* m_bucketAct;
    bool m_bucketMode{false};
    QAction* m_colorPickerAct;
    QColor m_bucketColor{Qt::black};

    // Sélection active pour l'image
    Selection m_selection;
    // remplissage pot de peinture
    void bucketFillAt(const QPoint& viewportPos);
    void updateColorPickerIcon();
    // Historique des commandes (undo/redo)
    app::History m_history{128};
    QAction* m_undoAct{nullptr};
    QAction* m_redoAct{nullptr};
};
