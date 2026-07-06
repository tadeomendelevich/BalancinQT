#include "odomchartview.h"
#include <QtCharts/QChart>
#include <QCursor>

OdomChartView::OdomChartView(QWidget *parent) : QChartView(parent)
{
    setDragMode(QGraphicsView::NoDrag);   // paneo manual vía scroll() del chart, no de la QGraphicsView
    setRubberBand(QChartView::NoRubberBand);
    setMouseTracking(true);
}

void OdomChartView::wheelEvent(QWheelEvent *event)
{
    if (!chart()) { QChartView::wheelEvent(event); return; }

    // Zoom centrado en el cursor: se calcula un rectángulo del área del
    // gráfico reducido/ampliado alrededor de la posición relativa del mouse
    // dentro del plot area, y se le pide al chart que haga zoom a ese rect.
    const double factor = (event->angleDelta().y() > 0) ? 0.85 : (1.0 / 0.85);

    const QRectF plotArea = chart()->plotArea();
    const QPointF mousePos = event->position();

    if (!plotArea.contains(mousePos)) {
        event->accept();
        return;
    }

    const double relX = (mousePos.x() - plotArea.left()) / plotArea.width();
    const double relY = (mousePos.y() - plotArea.top())  / plotArea.height();

    const QRectF zoomRect(
        plotArea.x() + plotArea.width()  * (1.0 - factor) * relX,
        plotArea.y() + plotArea.height() * (1.0 - factor) * relY,
        plotArea.width()  * factor,
        plotArea.height() * factor
        );

    chart()->zoomIn(zoomRect);
    emit viewChanged();
    event->accept();
}

void OdomChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && chart()) {
        m_panning     = true;
        m_lastMousePos = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QChartView::mousePressEvent(event);
}

void OdomChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning && chart()) {
        const QPoint currentPos = event->position().toPoint();
        const QPoint delta = currentPos - m_lastMousePos;
        // scroll() mueve la ventana de los ejes, no la vista; el signo hace
        // que arrastrar hacia la derecha "corra" el mapa como si lo tironeáramos.
        chart()->scroll(-delta.x(), delta.y());
        m_lastMousePos = currentPos;
        emit viewChanged();
        event->accept();
        return;
    }
    QChartView::mouseMoveEvent(event);
}

void OdomChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QChartView::mouseReleaseEvent(event);
}

void OdomChartView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (chart()) {
        chart()->zoomReset();
        emit viewChanged();
    }
    event->accept();
}
