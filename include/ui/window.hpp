#pragma once

#include <QAction>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QScrollArea>
#include <QToolBar>

#include <core/Selection.hpp>

#include "ui/image.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT
    friend class ImageActions;

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

    // Accès à la sélection
    Selection& selection() noexcept
    {
        return m_selection_;
    }
    const Selection& selection() const noexcept
    {
        return m_selection_;
    }

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
    QAction* m_openEpgAct;
    QAction* m_saveEpgAct;
    QAction* m_selectRectAct;
    QAction* m_clearSelectionAct;
    QAction* m_selectToggleAct;

    // Sélection active pour l'image
    Selection m_selection_;
};
