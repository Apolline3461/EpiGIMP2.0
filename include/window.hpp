#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP
#include <QInputDialog>
#include <QMainWindow>
#include <QLabel>
#include <QScrollArea>
#include <QImage>
#include <QPixmap>
#include <QMenu>
#include <QAction>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void newImage();
    void openImage();
    void saveImage();
    void closeImage();
    void zoomIn();
    void zoomOut();
    void resetZoom();

private:
    void createActions();
    void createMenus();
    void updateImageDisplay();
    void scaleImage(double factor);

    // Widgets
    QLabel *m_imageLabel;
    QScrollArea *m_scrollArea;
    
    // Données
    QImage m_currentImage;
    QString m_currentFileName;
    double m_scaleFactor;

    // Menus
    QMenu *m_fileMenu;
    QMenu *m_viewMenu;
    
    // Actions
    QAction *m_openAct;
    QAction *m_newAct;
    QAction *m_saveAct;
    QAction *m_closeAct;
    QAction *m_exitAct;
    QAction *m_zoomInAct;
    QAction *m_zoomOutAct;
    QAction *m_resetZoomAct;
};

#endif // MAINWINDOW_HPP