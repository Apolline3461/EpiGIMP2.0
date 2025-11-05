#pragma once

#include <QAction>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QScrollArea>

class MainWindow : public QMainWindow
{
    Q_OBJECT
    friend class ImageActions;

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

   private slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

    void openEpg();
    void saveAsEpg();

   private:
    void createActions();
    void createMenus();
    void updateImageDisplay();
    void scaleImage(double factor);

    bool saveEpgFormat(const QString& fileName);
    bool openEpgFormat(const QString& fileName);

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
    QAction* m_openEpgAct;
    QAction* m_saveEpgAct;
};
