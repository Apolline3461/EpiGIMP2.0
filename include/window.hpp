#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QAction>
#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPixmap>
#include <QScrollArea>

class MainWindow : public QMainWindow
{
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   private slots:
    void newImage();
    void openImage();
    void saveImage();
    void saveAsEpg();
    void openEpg();
    void closeImage();
    void zoomIn();
    void zoomOut();
    void resetZoom();

   private:
    void createActions();
    void createMenus();
    void updateImageDisplay();
    void scaleImage(double factor);
    bool openEpgFormat(const QString& fileName);
    bool saveEpgFormat(const QString& fileName);

    // Widgets
    QLabel* m_imageLabel;
    QScrollArea* m_scrollArea;

    // Données
    QImage m_currentImage;
    QString m_currentFileName;
    double m_scaleFactor;

    // Menus
    QMenu* m_fileMenu;
    QMenu* m_viewMenu;

    // Actions
    QAction* m_newAct;
    QAction* m_openAct;
    QAction* m_openEpgAct;
    QAction* m_saveAct;
    QAction* m_saveEpgAct;
    QAction* m_closeAct;
    QAction* m_exitAct;
    QAction* m_zoomInAct;
    QAction* m_zoomOutAct;
    QAction* m_resetZoomAct;
};

#endif  // MAINWINDOW_HPP