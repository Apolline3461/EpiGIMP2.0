//
// Created by apolline on 14/02/2026.
//

#pragma once
#include <QImage>
#include <QPointF>
#include <QWidget>

#include <optional>

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
    void setSelectionRectOverlay(std::optional<common::Rect> r);

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

   signals:
    void selectionFinishedDoc(common::Rect r);
    void clickedDoc(common::Point p);
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
    std::optional<common::Rect> selectionOverlay_;

    void drawChecker(QPainter& p);
};
