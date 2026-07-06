#include "healthdashboard.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>

namespace {
// Mirror de eRobotState (Core/Src/main.c del firmware). Frágil: si se reordena
// el enum en el firmware, actualizar esta tabla también.
QString robotStateName(quint8 s)
{
    switch (s) {
    case 0: return "IDLE";
    case 1: return "BALANCE";
    case 2: return "BALANCE+VEL";
    case 3: return "SEGUIDOR LINEA";
    case 4: return "MANUAL";
    case 5: return "TEST MOTORES";
    default: return QString("? (%1)").arg(s);
    }
}
}

HealthDashboard::HealthDashboard(QWidget *parent) : QFrame(parent)
{
    // Embebido en el panel de controles (debajo de Device PORT), no flotante —
    // compacto a propósito para no competir por espacio con el resto de la UI.
    setFrameShape(QFrame::StyledPanel);
    setStyleSheet(
        "HealthDashboard {"
        "  background-color: rgba(20, 20, 25, 200);"
        "  border: 1px solid rgba(255,255,255,50);"
        "  border-radius: 6px;"
        "}"
        "QLabel { color: #cfcfcf; background: transparent; }"
        );

    QFont f = font();
    f.setPointSize(8);
    setFont(f);

    // El modo del robot es el dato más importante para el usuario a simple
    // vista — grande, en negrita y separado del resto de las estadísticas.
    m_lblMode = new QLabel("MODO: --", this);
    QFont fm = m_lblMode->font();
    fm.setBold(true);
    fm.setPointSize(11);
    m_lblMode->setFont(fm);
    m_lblMode->setAlignment(Qt::AlignCenter);
    m_lblMode->setStyleSheet("color: #ffffff; padding: 2px;");

    m_dotConn = new QLabel(this);
    m_dotConn->setFixedSize(10, 10);
    setDotColor(m_dotConn, Qt::red);

    m_lblConn      = new QLabel("Sin datos", this);
    m_lblPackets   = new QLabel("Pkts: 0", this);
    m_lblRate      = new QLabel("Tasa: -- /s", this);
    m_lblLoss      = new QLabel("Pérdida: --", this);
    m_lblLatency   = new QLabel("Lat: -- ms", this);
    m_lblLastData  = new QLabel("Últ. dato: --", this);
    m_lblLastAlarm = new QLabel("Alarma: (ninguna)", this);
    m_lblLastAlarm->setWordWrap(true);

    auto *connRow = new QHBoxLayout();
    connRow->setSpacing(4);
    connRow->addWidget(m_dotConn);
    connRow->addWidget(m_lblConn, 1);

    // Grilla 2 columnas para que las estadísticas chicas ocupen poco alto.
    auto *statsGrid = new QGridLayout();
    statsGrid->setHorizontalSpacing(8);
    statsGrid->setVerticalSpacing(1);
    statsGrid->addWidget(m_lblPackets,  0, 0);
    statsGrid->addWidget(m_lblRate,     0, 1);
    statsGrid->addWidget(m_lblLatency,  1, 0, 1, 2);
    statsGrid->addWidget(m_lblLoss,     2, 0, 1, 2);
    statsGrid->addWidget(m_lblLastData, 3, 0, 1, 2);

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(m_lblMode);
    outer->addLayout(connRow);
    outer->addLayout(statsGrid);
    outer->addWidget(m_lblLastAlarm);
    outer->setContentsMargins(6, 5, 6, 5);
    outer->setSpacing(2);

    connect(&m_tickTimer, &QTimer::timeout, this, &HealthDashboard::tick);
    m_tickTimer.start(1000);
}

void HealthDashboard::setDotColor(QLabel *dot, const QColor &color)
{
    dot->setStyleSheet(QString("background-color: %1; border-radius: 6px;").arg(color.name()));
}

QString HealthDashboard::formatElapsedShort(qint64 ms)
{
    if (ms < 0) return "--";
    if (ms < 1000) return QString("%1 ms").arg(ms);
    if (ms < 60000) return QString("%1 s").arg(ms / 1000.0, 0, 'f', 1);
    return QString("%1 min").arg(ms / 60000.0, 0, 'f', 1);
}

void HealthDashboard::onPacketReceived()
{
    m_everReceived = true;
    m_packetCount++;
    m_packetsInWindow++;
    m_lastPacketTimer.start();
}

void HealthDashboard::onOdomSeq(quint16 seq)
{
    if (!m_hasSeq) {
        m_hasSeq        = true;
        m_lastSeq       = seq;
        m_expectedTotal = 1;
        return;
    }

    // uint16_t resta con wraparound natural: si el firmware reinicia el
    // contador (o hace overflow a los 65536 paquetes), la resta sigue dando
    // la distancia correcta mientras no se pierdan miles de paquetes seguidos.
    const quint16 delta = static_cast<quint16>(seq - m_lastSeq);
    if (delta == 0) return; // duplicado, no debería pasar pero por las dudas

    m_expectedTotal += delta;
    if (delta > 1) m_lostTotal += (delta - 1);
    m_lastSeq = seq;
}

void HealthDashboard::onRobotState(quint8 robotState)
{
    m_lblMode->setText("MODO: " + robotStateName(robotState));

    // Color de fondo distinto por modo — ayuda a notar un cambio de estado de
    // un vistazo, sin tener que leer el texto.
    QString bg;
    switch (robotState) {
    case 0: bg = "#555555"; break; // IDLE
    case 1:
    case 2: bg = "#1b5e20"; break; // BALANCE / BALANCE+VEL (verde)
    case 3: bg = "#0d47a1"; break; // SEGUIDOR LINEA (azul)
    case 4: bg = "#e65100"; break; // MANUAL (naranja)
    case 5: bg = "#4a148c"; break; // TEST MOTORES (violeta)
    default: bg = "#555555"; break;
    }
    m_lblMode->setStyleSheet(QString("color: #ffffff; padding: 2px; background-color: %1; border-radius: 4px;").arg(bg));
}

void HealthDashboard::onAlivePong(qint64 rttMs)
{
    m_lastLatencyMs = rttMs;
    m_lblLatency->setText(QString("Lat: ~%1 ms").arg(rttMs));
}

void HealthDashboard::onEvent(const QString &text)
{
    m_lastAlarmText = text;
    m_hasAlarm       = true;
    m_lastAlarmTimer.start();
    m_lblLastAlarm->setText("Alarma: " + text + " (recién)");
}

void HealthDashboard::setConnectionOpen(bool open)
{
    m_connectionOpen = open;
    if (!open) {
        m_everReceived = false;
        setDotColor(m_dotConn, Qt::gray);
        m_lblConn->setText("UDP cerrado");
        m_lblLastData->setText("Últ. dato: --");
    }
}

void HealthDashboard::tick()
{
    // Tasa de paquetes: acumulado del último segundo (el timer corre cada 1000ms).
    const double rate = m_packetsInWindow;
    m_packetsInWindow = 0;

    m_lblPackets->setText(QString("Pkts: %1").arg(m_packetCount));
    m_lblRate->setText(QString("Tasa: %1/s").arg(rate, 0, 'f', 1));

    if (m_expectedTotal > 0) {
        const double lossPct = 100.0 * m_lostTotal / m_expectedTotal;
        m_lblLoss->setText(QString("Pérdida: %1/%2 (%3%)")
                                .arg(m_lostTotal)
                                .arg(m_expectedTotal)
                                .arg(lossPct, 0, 'f', 1));
    }

    if (!m_connectionOpen) {
        setDotColor(m_dotConn, Qt::gray);
        m_lblConn->setText("UDP cerrado");
    } else if (!m_everReceived) {
        setDotColor(m_dotConn, Qt::red);
        m_lblConn->setText("Sin datos");
        m_lblLastData->setText("Últ. dato: --");
    } else {
        const qint64 elapsed = m_lastPacketTimer.elapsed();
        m_lblLastData->setText("Últ. dato: hace " + formatElapsedShort(elapsed));

        if (elapsed < 3000) {
            setDotColor(m_dotConn, Qt::green);
            m_lblConn->setText("Conectado");
        } else if (elapsed < 10000) {
            setDotColor(m_dotConn, QColor(255, 193, 7)); // amarillo
            m_lblConn->setText("Posible corte");
        } else {
            setDotColor(m_dotConn, Qt::red);
            m_lblConn->setText("Desconectado");
        }
    }

    if (m_hasAlarm) {
        m_lblLastAlarm->setText("Alarma: " + m_lastAlarmText
                                 + " (hace " + formatElapsedShort(m_lastAlarmTimer.elapsed()) + ")");
    }
}
