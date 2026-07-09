#ifndef ODOMCHARTVIEW_H
#define ODOMCHARTVIEW_H

#include <QtCharts/QChartView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPoint>

// QChartView especializado para el mapa de odometría: agrega zoom con la
// rueda del mouse (centrado en el cursor) y paneo arrastrando con el botón
// izquierdo, algo que QChartView no ofrece de fábrica (su drag mode por
// defecto no reescala los ejes, solo scrollea la QGraphicsView subyacente,
// que no tiene área extra para scrollear ya que el chart llena la vista).
//
// Además: hover del cursor (para leer coordenadas en metros) y un modo de
// MEDICIÓN estilo CAD — activado, el click izquierdo deja de panear y emite
// los puntos para medir distancias sobre el mapa.
class OdomChartView : public QChartView
{
    Q_OBJECT
public:
    explicit OdomChartView(QWidget *parent = nullptr);

    void setMeasureMode(bool on);
    bool measureMode() const { return m_measureMode; }

signals:
    // Emitida después de cualquier pan/zoom/reset, para que quien dibuje
    // anotaciones fuera de las series (QGraphicsItems sueltos en la escena)
    // sepa que debe recalcular su posición en pantalla.
    void viewChanged();

    // Posición del mouse dentro de la vista (coordenadas de viewport), en cada
    // movimiento — quien la recibe la mapea a valores de ejes (metros).
    void hoverAt(QPointF viewPos);

    // Click izquierdo con el modo medición activo (coordenadas de viewport).
    void measureClick(QPointF viewPos);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    bool   m_panning = false;
    bool   m_measureMode = false;
    QPoint m_lastMousePos;
};

#endif // ODOMCHARTVIEW_H
