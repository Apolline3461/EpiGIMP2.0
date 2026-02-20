// Created by apolline on 14/02/2026.

#include "ui/CanvasWidget.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>
#include <QWheelEvent>

#include <algorithm>

#include "ui/PanClamp.hpp"

namespace
{
static double clampScale(double s)
{
    return std::clamp(s, 0.10, 8.0);
}

static common::Rect makeRectFromTwoPoints(common::Point a, common::Point b)
{
    const int x1 = std::min(a.x, b.x);
    const int y1 = std::min(a.y, b.y);
    const int x2 = std::max(a.x, b.x);
    const int y2 = std::max(a.y, b.y);

    common::Rect r{};
    r.x = x1;
    r.y = y1;
    r.w = (x2 - x1);
    r.h = (y2 - y1);
    return r;
}

static common::Rect clampRectToImage(common::Rect r, int w, int h)
{
    if (w <= 0 || h <= 0)
        return common::Rect{0, 0, 0, 0};

    int x = std::clamp(r.x, 0, w);
    int y = std::clamp(r.y, 0, h);

    int xEnd = std::clamp(r.x + r.w, 0, w);
    int yEnd = std::clamp(r.y + r.h, 0, h);

    common::Rect out{};
    out.x = std::min(x, xEnd);
    out.y = std::min(y, yEnd);
    out.w = std::max(0, std::abs(xEnd - x));
    out.h = std::max(0, std::abs(yEnd - y));
    return out;
}
}  // namespace

CanvasWidget::CanvasWidget(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void CanvasWidget::setImage(const QImage& img)
{
    img_ = img;
    update();
}

void CanvasWidget::clear()
{
    img_ = QImage();
    hasSel_ = false;
    selScreen_ = QRect();
    resetView();
    update();
}

void CanvasWidget::setSelectionEnable(bool enable)
{
    // Tu dois ajouter `bool selectionEnabled_ = false;` dans le .hpp
    selectionEnabled_ = enable;
    if (!selectionEnabled_)
    {
        hasSel_ = false;
        selScreen_ = QRect();
        update();
    }
}

void CanvasWidget::setSelectionRectOverlay(std::optional<common::Rect> r)
{
    selectionOverlay_ = r;
    update();
}

void CanvasWidget::setSelectionRect(const QRect& r)
{
    hasSel_ = !r.isNull() && r.isValid();
    selScreen_ = r;
    update();
}

void CanvasWidget::clearSelectionRect()
{
    hasSel_ = false;
    selScreen_ = QRect();
    selectionOverlay_.reset();
    update();
}

void CanvasWidget::setScale(double s)
{
    scale_ = clampScale(s);
    clampPan();
    update();
}

void CanvasWidget::resetView()
{
    scale_ = 1.0;
    pan_ = QPointF(0, 0);
    update();
}

void CanvasWidget::setPan(QPointF p)
{
    pan_ = p;
    update();
}

QPointF CanvasWidget::pan() const
{
    return pan_;
}

common::Point CanvasWidget::screenToDoc(const QPoint& sp) const
{
    const double x = (sp.x() - pan_.x()) / scale_;
    const double y = (sp.y() - pan_.y()) / scale_;
    return common::Point{static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y))};
}

QPoint CanvasWidget::docToScreen(common::Point p) const
{
    const double x = pan_.x() + static_cast<double>(p.x) * scale_;
    const double y = pan_.y() + static_cast<double>(p.y) * scale_;
    return QPoint(static_cast<int>(std::round(x)), static_cast<int>(std::round(y)));
}

void CanvasWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    drawChecker(p);

    if (img_.isNull())
        return;

    p.save();
    p.translate(pan_);
    p.scale(scale_, scale_);
    p.drawImage(0, 0, img_);

    if (selectionOverlay_)
    {
        QPen pen(QColor(220, 0, 0));
        pen.setStyle(Qt::DashLine);
        pen.setWidth(1 / scale_);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        const auto& r = *selectionOverlay_;
        p.drawRect(r.x, r.y, r.w, r.h);
    }

    p.restore();

    if (hasSel_)
    {
        QPen pen(QColor(220, 0, 0));
        pen.setStyle(Qt::DashLine);
        pen.setWidth(1);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(selScreen_);
    }
}

void CanvasWidget::wheelEvent(QWheelEvent* e)
{
    if (img_.isNull())
    {
        e->ignore();
        return;
    }

    const QPointF mousePos = e->position();
    const double oldScale = scale_;

    const double steps = e->angleDelta().y() / 120.0;
    const double factor = std::pow(1.15, steps);

    double newScale = clampScale(oldScale * factor);
    if (qFuzzyCompare(newScale, oldScale))
    {
        e->accept();
        return;
    }

    const double docX = (mousePos.x() - pan_.x()) / oldScale;
    const double docY = (mousePos.y() - pan_.y()) / oldScale;

    scale_ = newScale;
    pan_.setX(mousePos.x() - docX * scale_);
    pan_.setY(mousePos.y() - docY * scale_);
    clampPan();
    update();
    e->accept();
}

void CanvasWidget::mousePressEvent(QMouseEvent* e)
{
    lastMouse_ = e->pos();

    if (e->button() == Qt::MiddleButton || e->button() == Qt::RightButton)
    {
        panning_ = true;
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }

    if (e->button() == Qt::LeftButton)
    {
        if (!img_.isNull() && selectionEnabled_)
        {
            hasSel_ = true;
            selScreen_ = QRect(e->pos(), e->pos());
            e->accept();
            update();
            return;
        }

        // clic outil (bucket etc.)
        if (!img_.isNull())
        {
            common::Point pDoc = screenToDoc(e->pos());
            emit clickedDoc(pDoc);
        }
        e->accept();
        return;
    }

    QWidget::mousePressEvent(e);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* e)
{
    const QPoint cur = e->pos();
    const QPoint delta = cur - lastMouse_;
    lastMouse_ = cur;

    if (panning_)
    {
        pan_ += QPointF(delta);
        clampPan();
        update();
        e->accept();
        return;
    }

    if (selectionEnabled_ && hasSel_ && (e->buttons() & Qt::LeftButton))
    {
        selScreen_.setBottomRight(cur);
        update();
        e->accept();
        return;
    }

    QWidget::mouseMoveEvent(e);
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::MiddleButton || e->button() == Qt::RightButton)
    {
        if (panning_)
        {
            panning_ = false;
            unsetCursor();
            e->accept();
            return;
        }
    }

    if (e->button() == Qt::LeftButton)
    {
        if (selectionEnabled_ && hasSel_)
        {
            // Normalise screen selection
            selScreen_ = selScreen_.normalized();

            if (!img_.isNull())
            {
                const QPoint tl = selScreen_.topLeft();
                const QPoint br = selScreen_.bottomRight();

                common::Point a = screenToDoc(tl);
                common::Point b = screenToDoc(br);

                common::Rect r = makeRectFromTwoPoints(a, b);
                r = clampRectToImage(r, img_.width(), img_.height());

                hasSel_ = false;
                selScreen_ = QRect();

                if (r.w <= 0 || r.h <= 0)
                {
                    emit selectionFinishedDoc(common::Rect{0, 0, 0, 0});  // ou n’émet pas du tout
                }
                else
                {
                    emit selectionFinishedDoc(r);
                }
                update();
            }
            e->accept();
            return;
        }
    }

    QWidget::mouseReleaseEvent(e);
}

void CanvasWidget::drawChecker(QPainter& p)
{
    const int tile = 16;
    QColor c1(200, 200, 200);
    QColor c2(230, 230, 230);

    for (int y = 0; y < height(); y += tile)
    {
        for (int x = 0; x < width(); x += tile)
        {
            const bool odd = ((x / tile) + (y / tile)) % 2;
            p.fillRect(QRect(x, y, tile, tile), odd ? c1 : c2);
        }
    }
}

void CanvasWidget::clampPan()
{
    if (img_.isNull())
        return;

    const auto r = computeClampedPan(pan_.x(), pan_.y(), img_.width(), img_.height(), scale_,
                                     width(), height(), 32.0);

    pan_.setX(r.x);
    pan_.setY(r.y);
}
