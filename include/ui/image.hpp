#pragma once
#include <QLabel>
#include <QPoint>
#include <QRect>

class MainWindow;
class QMouseEvent;
class QRubberBand;

class ImageActions
{
   public:
    static void newImage(MainWindow* window);
    static void openImage(MainWindow* window);
    static void saveImage(MainWindow* window);
    static void closeImage(MainWindow* window);
};

class ImageLabel : public QLabel
{
    Q_OBJECT
   public:
    explicit ImageLabel(QWidget* parent = nullptr);

   signals:
    void selectionFinished(const QRect& rect);

   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

   private:
    QPoint m_origin_;
    QRubberBand* m_rubberBand_ = nullptr;
};
