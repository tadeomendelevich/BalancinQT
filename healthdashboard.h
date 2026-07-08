#ifndef HEALTHDASHBOARD_H
#define HEALTHDASHBOARD_H

#include <QFrame>
#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>

// Panel de salud del sistema: estado de conexión WiFi, paquetes recibidos +
// tasa, pérdida de paquetes, latencia, tiempo desde el último dato y modo
// actual del robot (destacado, es el dato más importante para el usuario).
// Diseñado compacto para vivir embebido en el panel de controles (debajo de
// Device PORT), no como overlay flotante — ver MainWindow::MainWindow()
// donde se hace `ui->verticalLayout->addWidget(...)`.
class HealthDashboard : public QFrame
{
    Q_OBJECT
public:
    explicit HealthDashboard(QWidget *parent = nullptr);

    void onPacketReceived();             // cualquier datagrama UDP recibido (válido o no)
    void onOdomSeq(quint16 seq);         // seq del paquete WIFI_ODOM_DATA=0xDC, para medir pérdida
    void onRobotState(quint8 robotState); // robot_state del mismo paquete (eRobotState del firmware)
    void onAlivePong(qint64 rttMs);      // round-trip de un ping GETALIVE
    void setConnectionOpen(bool open);   // el usuario abrió/cerró el socket UDP manualmente

private slots:
    void tick(); // refresco de UI ~1x/seg: tasa de paquetes, tiempo transcurrido, color del estado

private:
    void setDotColor(QLabel *dot, const QColor &color);
    static QString formatElapsedShort(qint64 ms);

    QLabel *m_dotConn;
    QLabel *m_lblConn;
    QLabel *m_lblPackets;
    QLabel *m_lblRate;
    QLabel *m_lblLoss;
    QLabel *m_lblLatency;
    QLabel *m_lblLastData;
    QLabel *m_lblMode;

    QTimer        m_tickTimer;
    QElapsedTimer m_lastPacketTimer;
    bool          m_everReceived    = false;
    bool          m_connectionOpen  = false;

    quint64 m_packetCount     = 0;
    quint64 m_packetsInWindow = 0; // acumulado desde el último tick(), define la tasa/seg

    bool    m_hasSeq        = false;
    quint16 m_lastSeq       = 0;
    quint64 m_expectedTotal = 0;
    quint64 m_lostTotal     = 0;

    qint64 m_lastLatencyMs = -1;
};

#endif // HEALTHDASHBOARD_H
