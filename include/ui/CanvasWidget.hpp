//
// Created by apolline on 14/02/2026.
//

#pragma once
#include <QColor>
#include <QImage>
#include <QPointF>
#include <QWidget>

#include <optional>
#include <vector>

#include "common/Geometry.hpp"

class CanvasWidget : public QWidget
{
    Q_OBJECT
   public:
    explicit CanvasWidget(QWidget* parent = nullptr);

    bool hasImage() const
    {
        return !img_.isNull();
    }
    QSize imageSize() const
    {
        return img_.size();
    }
    void setImage(const QImage& img);
    void clear();

    void setSelectionEnable(bool enable);

    void setSelectionRect(const QRect& r);
    void clearSelectionRect();

    double scale() const
    {
        return scale_;
    }
    void setScale(double s);
    void resetView();
    void setPan(QPointF p);
    QPointF pan() const;

    // conversions
    common::Point screenToDoc(const QPoint& sp) const;
    QPoint docToScreen(common::Point p) const;
    void setBrushSize(int s)
    {
        brushSize_ = s;
    }
    void setBrushColor(const QColor& c)
    {
        brushColor_ = c;
    }

   signals:
    void selectionFinishedDoc(common::Rect r);
    void clickedDoc(common::Point p);
    void beginStroke(common::Point p);
    void moveStroke(common::Point p);
    void endStroke();
    // void beginDragDoc(common::Point p);
    // void dragDoc(common::Point p);
    // void endDragDoc(common::Point p);

   protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

   private:
    QImage img_;
    double scale_ = 1.0;
    QPointF pan_ = {0, 0};
    bool panning_ = false;
    QPoint lastMouse_;

    // selection overlay simple
    bool hasSel_ = false;
    QRect selScreen_;
    bool selectionEnabled_ = false;
    bool drawing_ = false;
    std::vector<common::Point> previewPoints_;
    QColor brushColor_ = QColor(0, 0, 0, 255);
    int brushSize_ = 1;

    void drawChecker(QPainter& p);
};
