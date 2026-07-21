#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork/QNetworkDatagram>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <QFileDialog>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QtMath>
#include <cmath>
#include <algorithm>
#include <QStyle>
#include <QTabBar>
#include <QtCharts/QAbstractAxis>

// ============================================================
// Helpers de estética — el tema global de la app vive en theme.qss
// (recurso :/style/theme.qss, cargado en main.cpp); acá va solamente
// lo que un stylesheet no puede cubrir: QCharts y propiedades dinámicas.
// ============================================================
namespace {

// Estado visual de los botones toggle (BALANCE / SEGUIR LÍNEA / Abrir UDP).
// theme.qss define la apariencia para toggleState="on" (verde) y "off" (rojo);
// re-polish fuerza a Qt a reevaluar el selector tras cambiar la propiedad.
void setToggleState(QWidget *w, const char *state)
{
    w->setProperty("toggleState", QString::fromLatin1(state));
    w->style()->unpolish(w);
    w->style()->polish(w);
}

// Estilo unificado de todos los QChart, con los mismos colores del tema
// (los QChart no son estilables por QSS — hay que pintarlos por API).
void styleChart(QChart *chart)
{
    const QColor surface("#151b23");
    const QColor plotBg("#0c1117");
    const QColor text("#d8dfe8");
    const QColor dim("#8a97a8");
    const QColor grid("#232c38");

    chart->setBackgroundBrush(surface);
    chart->setBackgroundPen(Qt::NoPen);
    chart->setBackgroundRoundness(8);
    chart->setPlotAreaBackgroundBrush(plotBg);
    chart->setPlotAreaBackgroundVisible(true);
    chart->setTitleBrush(QBrush(text));
    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(10);
    titleFont.setBold(true);
    chart->setTitleFont(titleFont);
    chart->legend()->setLabelColor(dim);
    const auto axes = chart->axes();
    for (QAbstractAxis *axis : axes) {
        axis->setLabelsBrush(QBrush(dim));
        axis->setTitleBrush(QBrush(dim));
        axis->setLinePen(QPen(grid));
        axis->setGridLinePen(QPen(grid));
        axis->setMinorGridLinePen(QPen(grid));
    }
}

// Encabezados de las consolas de texto (mismos colores del tema) — usado al
// limpiar pantallas tanto desde el botón como desde el watchdog UDP.
void resetConsoleHeaders(QTextEdit *rawEdit, QTextEdit *procEdit)
{
    rawEdit->setHtml(
        "<html><head><meta charset=\"utf-8\"/></head>"
        "<body style=\"font-family:'Consolas'; font-size:12px;\">"
        "<p align=\"center\" style=\"margin:8px 4px;\">"
        "<span style=\"font-weight:bold; color:#8a97a8; letter-spacing:2px;\">&mdash; DATO ENVIADO &mdash;</span>"
        "</p></body></html>"
    );
    procEdit->setHtml(
        "<html><head><meta charset=\"utf-8\"/></head>"
        "<body style=\"font-family:'Consolas'; font-size:13px;\">"
        "<p align=\"center\" style=\"margin:8px 4px;\">"
        "<span style=\"font-weight:bold; color:#79c0ff; letter-spacing:2px;\">&mdash; DATO RECIBIDO &mdash;</span>"
        "</p></body></html>"
    );
}

// Grupo plegable: el indicador del título del QGroupBox muestra/oculta todo
// su contenido (los widgets ocultos no reservan espacio en el layout).
void makeCollapsible(QGroupBox *box, bool startOpen)
{
    box->setCheckable(true);
    auto apply = [box](bool open) {
        const auto children = box->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *w : children) w->setVisible(open);
    };
    QObject::connect(box, &QGroupBox::toggled, box, apply);
    box->setChecked(startOpen);
    apply(startOpen);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->accelerationWidget->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    ui->gyroscopeWidget->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    this->setFocusPolicy(Qt::StrongFocus);
    qApp->installEventFilter(this);
    serial=new QSerialPort(this);
    settingPorts=new SettingsDialog(this);
    estadoSerial = new QLabel(this);
    estadoSerial->setText("Desconectado ......");
    ui->statusbar->addWidget(estadoSerial);
    ui->actionDisconnect_Device->setEnabled(false);
    timer1=new QTimer(this);
    UdpSocket1 = new QUdpSocket(this);

    manualControlTimer = new QTimer(this);
    connect(manualControlTimer, &QTimer::timeout, this, &MainWindow::sendManualCommand);

    udpWatchdogTimer = new QTimer(this);
    connect(udpWatchdogTimer, &QTimer::timeout, this, &MainWindow::checkUdpInactivity);
    udpEverReceivedData = false;

    // Dashboard de salud: embebido en el panel de controles, justo debajo de
    // Device PORT (mismo lugar donde se abre/configura la conexión UDP) — la
    // versión flotante arriba a la derecha tapaba otras cosas y molestaba.
    healthDashboard = new HealthDashboard(this);
    healthDashboard->setConnectionOpen(false);
    ui->verticalLayout->addWidget(healthDashboard);

    alivePingTimer = new QTimer(this);
    connect(alivePingTimer, &QTimer::timeout, this, &MainWindow::sendAlivePing);

    connect(ui->textEdit_RAW, &QTextEdit::textChanged, this, [this](){
        ui->textEdit_RAW->moveCursor(QTextCursor::End);
    });
    connect(ui->textEdit_PROCCES, &QTextEdit::textChanged, this, [this](){
        ui->textEdit_PROCCES->moveCursor(QTextCursor::End);
    });

    // Las consolas pasan a ser cajones al final del tablero scrolleable. Ambas
    // arrancan colapsadas para priorizar siempre la operacion y los graficos.
    QWidget *consoleDrawer = nullptr;
    {
        consoleDrawer = new QWidget(ui->widget_controls);
        consoleDrawer->setObjectName("dataConsoleDrawer");
        auto *drawerLayout = new QVBoxLayout(consoleDrawer);
        drawerLayout->setContentsMargins(0, 0, 0, 0);
        drawerLayout->setSpacing(2);

        auto *toggleBar = new QWidget(consoleDrawer);
        auto *toggleLayout = new QHBoxLayout(toggleBar);
        toggleLayout->setContentsMargins(4, 2, 4, 2);
        toggleLayout->setSpacing(6);
        auto *btnToggleReceived = new QPushButton("\u25b8 Dato recibido", toggleBar);
        auto *btnToggleRaw = new QPushButton("\u25b8 Dato enviado", toggleBar);
        btnToggleReceived->setObjectName("btnToggleReceived");
        btnToggleRaw->setObjectName("btnToggleRaw");
        btnToggleReceived->setToolTip("Mostrar u ocultar datos recibidos");
        btnToggleRaw->setToolTip("Mostrar u ocultar datos enviados");
        toggleLayout->addWidget(btnToggleReceived);
        toggleLayout->addWidget(btnToggleRaw);
        toggleLayout->addStretch(1);
        toggleBar->setFixedHeight(32);
        drawerLayout->addWidget(toggleBar);

        ui->splitter_texts->setParent(consoleDrawer);
        ui->splitter_texts->setOrientation(Qt::Horizontal);
        ui->splitter_texts->setMinimumSize(0, 0);
        ui->splitter_texts->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        ui->splitter_texts->setChildrenCollapsible(false);
        drawerLayout->addWidget(ui->splitter_texts, 1);

        auto updateDrawer = [this, consoleDrawer, btnToggleReceived, btnToggleRaw]() {
            // isVisible() tambien depende de la visibilidad del splitter padre;
            // isHidden() refleja solamente el estado explicito de cada consola.
            const bool receivedOpen = !ui->textEdit_PROCCES->isHidden();
            const bool rawOpen = !ui->textEdit_RAW->isHidden();
            const bool anyOpen = receivedOpen || rawOpen;
            ui->splitter_texts->setVisible(anyOpen);
            consoleDrawer->setMinimumHeight(anyOpen ? 170 : 34);
            consoleDrawer->setMaximumHeight(anyOpen ? 280 : 34);
            btnToggleReceived->setText(receivedOpen ? "\u25be Ocultar dato recibido"
                                                    : "\u25b8 Dato recibido");
            btnToggleRaw->setText(rawOpen ? "\u25be Ocultar dato enviado"
                                          : "\u25b8 Dato enviado");
        };

        connect(btnToggleReceived, &QPushButton::clicked, this,
                [this, updateDrawer]() {
            ui->textEdit_PROCCES->setVisible(ui->textEdit_PROCCES->isHidden());
            updateDrawer();
        });
        connect(btnToggleRaw, &QPushButton::clicked, this,
                [this, updateDrawer]() {
            ui->textEdit_RAW->setVisible(ui->textEdit_RAW->isHidden());
            updateDrawer();
        });

        ui->textEdit_PROCCES->hide();
        ui->textEdit_RAW->hide();
        updateDrawer();
    }

    // Tablero operativo en dos columnas balanceadas. Conexion, comandos y
    // joystick quedan juntos; los ajustes PID y Setpoint ocupan la otra columna.
    while (QLayoutItem *item = ui->verticalLayout_6->takeAt(0)) delete item;
    delete ui->verticalLayout_6;
    auto *controlsGrid = new QGridLayout(ui->widget_controls);
    controlsGrid->setObjectName("gridLayout_ControlsDashboard");
    controlsGrid->setContentsMargins(6, 4, 6, 4);
    controlsGrid->setHorizontalSpacing(10);
    controlsGrid->setVerticalSpacing(8);
    controlsGrid->addWidget(ui->groupBox_Conexion, 0, 0);
    controlsGrid->addWidget(ui->groupBox_Comandos, 1, 0);
    controlsGrid->addWidget(ui->groupBox_Manual, 2, 0);
    controlsGrid->addWidget(ui->groupBox_PID_Balance, 0, 1);
    controlsGrid->addWidget(ui->groupBox_PID_Line, 1, 1);
    controlsGrid->addWidget(ui->groupBox_Setpoint, 2, 1);
    controlsGrid->addWidget(consoleDrawer, 3, 0, 1, 2);
    controlsGrid->setColumnStretch(0, 1);
    controlsGrid->setColumnStretch(1, 1);
    controlsGrid->setRowStretch(4, 1);

    // Ajuste directo de la velocidad de crucero del seguidor. El protocolo ya
    // reservaba 0xC4; esta fila evita el dialogo generico y expresa la unidad real.
    {
        auto *speedRow = new QWidget(ui->groupBox_PID_Line);
        speedRow->setObjectName("lineSpeedRow");
        auto *speedLayout = new QGridLayout(speedRow);
        speedLayout->setContentsMargins(0, 4, 0, 0);
        speedLayout->setHorizontalSpacing(6);

        auto *speedLabel = new QLabel("Velocidad objetivo", speedRow);
        speedLabel->setToolTip("Velocidad maxima en recta; en curvas el firmware la reduce automaticamente");
        lineSpeedSpinBox = new QDoubleSpinBox(speedRow);
        lineSpeedSpinBox->setObjectName("spinLineSpeed");
        lineSpeedSpinBox->setRange(0.20, 4.00);
        lineSpeedSpinBox->setDecimals(2);
        lineSpeedSpinBox->setSingleStep(0.10);
        lineSpeedSpinBox->setSuffix(" m/s");
        lineSpeedSpinBox->setValue(2.50);
        lineSpeedSpinBox->setToolTip("Rango seguro aceptado por STM32: 0.20 a 4.00 m/s");

        auto *applySpeed = new QPushButton("APLICAR", speedRow);
        applySpeed->setObjectName("btnApplyLineSpeed");
        lineSpeedStatusLabel = new QLabel("Valor propuesto: 2.50 m/s", speedRow);
        lineSpeedStatusLabel->setObjectName("lblLineSpeedStatus");

        speedLayout->addWidget(speedLabel, 0, 0);
        speedLayout->addWidget(lineSpeedSpinBox, 0, 1);
        speedLayout->addWidget(applySpeed, 0, 2);
        speedLayout->addWidget(lineSpeedStatusLabel, 1, 0, 1, 3);
        ui->verticalLayout_PID_LINE->addWidget(speedRow);

        connect(applySpeed, &QPushButton::clicked, this, [this]() {
            requestedLineSpeedMps = static_cast<float>(lineSpeedSpinBox->value());
            if (sendLineSpeedCommand(requestedLineSpeedMps)) {
                lineSpeedStatusLabel->setText(
                    QString("Enviando %1 m/s...").arg(requestedLineSpeedMps, 0, 'f', 2));
            }
        });
    }

    ui->scrollArea_controls->setMinimumWidth(560);
    ui->scrollArea_controls->setMaximumWidth(900);
    ui->splitter_main->setStretchFactor(0, 6);
    ui->splitter_main->setStretchFactor(1, 5);
    makeCollapsible(ui->groupBox_PID_Balance, true);
    makeCollapsible(ui->groupBox_PID_Line, true);
    makeCollapsible(ui->groupBox_Setpoint, true);
    makeCollapsible(ui->groupBox_Manual, true);

    // Estado visual inicial del toggle de conexión (theme.qss: toggleState)
    setToggleState(ui->pushButton_OPENUDP, "off");

    // Conectar botones D-PAD a control manual
    connect(ui->btn_UP, &QPushButton::pressed, this, [=](){
        currentManualCommand = MOVE_FORWARD;
        ui->textEdit_PROCCES->append("MANUAL CMD: ADELANTE ↑ [0x" + QString("%1").arg(MOVE_FORWARD, 2, 16, QChar('0')).toUpper() + "]");
        sendManualCommand();
        if (!manualControlTimer->isActive()) manualControlTimer->start(50);
    });
    connect(ui->btn_UP, &QPushButton::released, this, [=](){
        manualControlTimer->stop();
        currentManualCommand = MOVE_STOP;
        ui->textEdit_PROCCES->append("MANUAL CMD: STOP ■ [0x" + QString("%1").arg(MOVE_STOP, 2, 16, QChar('0')).toUpper() + "]");
        sendManualCommand();
        currentManualCommand = 0;
    });

    connect(ui->btn_DOWN, &QPushButton::pressed, this, [=](){
        currentManualCommand = MOVE_BACKWARD;
        ui->textEdit_PROCCES->append("MANUAL CMD: ATRÁS ↓ [0x" + QString("%1").arg(MOVE_BACKWARD, 2, 16, QChar('0')).toUpper() + "]");
        sendManualCommand();
        if (!manualControlTimer->isActive()) manualControlTimer->start(50);
    });
    connect(ui->btn_DOWN, &QPushButton::released, this, [=](){
        manualControlTimer->stop();
        currentManualCommand = MOVE_STOP;
        ui->textEdit_PROCCES->append("MANUAL CMD: STOP ■ [0x" + QString("%1").arg(MOVE_STOP, 2, 16, QChar('0')).toUpper() + "]");
        sendManualCommand();
        currentManualCommand = 0;
    });

    connect(ui->btn_LEFT, &QPushButton::pressed, this, [=](){
        currentManualCommand = MOVE_LEFT;
        ui->textEdit_PROCCES->append("MANUAL CMD: IZQUIERDA ← [0x" + QString("%1").arg(MOVE_LEFT, 2, 16, QChar('0')).toUpper() + "]");
        sendManualCommand();
        if (!manualControlTimer->isActive()) manualControlTimer->start(50);
    });
    connect(ui->btn_LEFT, &QPushButton::released, this, [=](){
        manualControlTimer->stop();
        currentManualCommand = MOVE_STOP;
        ui->textEdit_PROCCES->append("MANUAL CMD: STOP ■ [0x" + QString("%1").arg(MOVE_STOP, 2, 16, QChar('0')).toUpper() + "]");
        sendManualCommand();
        currentManualCommand = 0;
    });

    connect(ui->btn_RIGHT, &QPushButton::pressed, this, [=](){
        currentManualCommand = MOVE_RIGHT;
        ui->textEdit_PROCCES->append("MANUAL CMD: DERECHA → [0x" + QString("%1").arg(MOVE_RIGHT, 2, 16, QChar('0')).toUpper() + "]");
        sendManualCommand();
        if (!manualControlTimer->isActive()) manualControlTimer->start(50);
    });
    connect(ui->btn_RIGHT, &QPushButton::released, this, [=](){
        manualControlTimer->stop();
        currentManualCommand = MOVE_STOP;
        ui->textEdit_PROCCES->append("MANUAL CMD: STOP ■ [0x" + QString("%1").arg(MOVE_STOP, 2, 16, QChar('0')).toUpper() + "]");
        sendManualCommand();
        currentManualCommand = 0;
    });

    connect(ui->actionQuit,&QAction::triggered,this,&MainWindow::close);
    connect(ui->actionScanPorts, &QAction::triggered, settingPorts,&SettingsDialog::show);
    connect(ui->actionConnect_Device, &QAction::triggered,this,&MainWindow::openSerialPorts);
    connect(ui->actionDisconnect_Device, &QAction::triggered, this, &MainWindow::closeSerialPorts);
    connect(ui->pushButton_SEND,&QPushButton::clicked, this, &MainWindow::sendDataSerial);
    connect(serial,&QSerialPort::readyRead,this,&MainWindow::dataRecived);
    connect(timer1,&QTimer::timeout,this,&MainWindow::timeOut);
    connect(UdpSocket1,&QUdpSocket::readyRead,this,&MainWindow::OnUdpRxData);
    connect(ui->pushButton_UDP,&QPushButton::clicked,this,&MainWindow::sendDataUDP);
    connect(ui->pushButton_RECORD, &QPushButton::clicked, this, &MainWindow::toggleRecording);
    connect(ui->btnSelectLogDir, &QPushButton::clicked, this, &MainWindow::selectSaveDirectory);

    // Configurar directorio por defecto (Documents/logs)
    defaultSaveDirectory = "C:/Users/tadeo/Desktop/Balancin_logs";
    // Si no existe, usar la carpeta de la app
    if(defaultSaveDirectory.isEmpty()) {
        defaultSaveDirectory = "logs";
    }
    // Crear la carpeta si no existe para que se vea bien
    QDir().mkpath(defaultSaveDirectory);
    ui->lineEdit_LogDir->setText(defaultSaveDirectory);

    ui->comboBox_CMD->addItem("ALIVE", 0xF0);
    ui->comboBox_CMD->addItem("GETADCVALUES", 0xA5);
    //ui->comboBox_CMD->addItem("MPU6050", 0xA6);
    ui->comboBox_CMD->addItem("ANGLE", 0xA7);
    ui->comboBox_CMD->addItem("MOTORES", 0xA1);
    ui->comboBox_CMD->addItem("VELOCIDAD", 0xA4);
    ui->comboBox_CMD->addItem("SENDALLSENSORS", 0xA9);
    ui->comboBox_CMD->addItem("MODIFICAR KP", 0xB1);
    ui->comboBox_CMD->addItem("MODIFICAR KD", 0xB2);
    ui->comboBox_CMD->addItem("MODIFICAR KI", 0xB3);
    ui->comboBox_CMD->addItem("BALANCE", 0xB4);
    //ui->comboBox_CMD->addItem("RESET CENTRO DE MASA", 0xB7);
    ui->comboBox_CMD->addItem("ACTIVAR LOG CSV", 0xB9);
    ui->comboBox_CMD->addItem("ACTIVAR WIFI LOG", 0xBA);
    ui->comboBox_CMD->addItem("MODIFICAR BETA_G", 0xBC);
    ui->comboBox_CMD->addItem("MODIFICAR BETA_A", 0xBD);
    ui->comboBox_CMD->addItem("SIGUIENTE PAGINA DISPLAY", 0xBE);
    ui->comboBox_CMD->addItem("MODIFICAR KV_BRAKE", 0xBF);
    ui->comboBox_CMD->addItem("MODIFICAR KP_LINE", 0xC0);
    ui->comboBox_CMD->addItem("MODIFICAR KD_LINE", 0xC1);
    ui->comboBox_CMD->addItem("MODIFICAR KI_LINE", 0xC2);
    ui->comboBox_CMD->addItem("MODIFICAR LINE_THRES", 0xC3);
    ui->comboBox_CMD->addItem("MODIFICAR VELOCIDAD DE AVANCE EN LINEA", 0xC4);
    ui->comboBox_CMD->addItem("ACTIVAR SEGUIDOR LINEA", 0xC5);
    ui->comboBox_CMD->addItem("ACTIVAR MANTENCION DE POSICION", 0xC6);
    ui->comboBox_CMD->addItem("ACTIVAR CONTROL MANUAL (MEDIANTE COMANDOS)", 0xC7);
    ui->comboBox_CMD->addItem("MODIFICAR SETPOINT", 0xC8);
    ui->comboBox_CMD->addItem("GET ODOMETRY", 0xDA);
    ui->comboBox_CMD->addItem("RESET ODOMETRY", 0xDB);

    estadoProtocolo=START;
    estadoProtocoloUdp = START;
    rxData.timeOut=0;
    ui->pushButton_UDP->setEnabled(false);
    ui->pushButton_BALANCE->setEnabled(false);
    ui->pushButton_FOLLOW_LINE->setEnabled(false);
    ui->pushButton_SEND->setEnabled(false);
    timer1->start(100);

    // 1) Asocia los QChartView de tu .ui
    accChartView  = ui->accelerationWidget;
    gyroChartView = ui->gyroscopeWidget;

    // 2) Crea y configura los QChart
    accChart  = new QChart();
    gyroChart = new QChart();
    accChartView->setChart(accChart);
    gyroChartView->setChart(gyroChart);
    accChartView->setRenderHint(QPainter::Antialiasing);
    gyroChartView->setRenderHint(QPainter::Antialiasing);

    // 3) Crea las series
    seriesAx = new QLineSeries(); seriesAx->setName("Aceleración Roll (Raw)");
    seriesAx->setColor(Qt::lightGray); // Cambiamos a gris para que el filtrado resalte mas

    seriesRollFilt = new QLineSeries(); seriesRollFilt->setName("Roll Filtrado (Fusion)");
    seriesRollFilt->setColor(Qt::red); // Rojo para el dato principal

    seriesAccelRollF = new QLineSeries(); seriesAccelRollF->setName("Roll Filtrado (Low Pass)");
    seriesAccelRollF->setColor(QColor(255, 165, 0)); // Naranja

    seriesPitch = new QLineSeries(); seriesPitch->setName("Pitch");
    seriesPitch->setColor(Qt::magenta);

    seriesDynSp = new QLineSeries(); seriesDynSp->setName("Setpoint Dinámico");
    seriesDynSp->setColor(QColor(0, 200, 0));  // Verde

    seriesGx = new QLineSeries(); seriesGx->setName("Giroscopio Y");
    seriesGx->setColor(Qt::blue);

    seriesGy = new QLineSeries(); seriesGy->setName("Gyr Y (reservado)");
    seriesGy->setColor(Qt::gray);
    seriesGz = new QLineSeries(); seriesGz->setName("Gyr Z (reservado)");
    seriesGz->setColor(Qt::lightGray);

    // 4) Añade sólo las series de aceleración al accChart
    accChart->addSeries(seriesAx);
    accChart->addSeries(seriesRollFilt); // Agregamos el filtrado
    accChart->addSeries(seriesAccelRollF); // Agregamos el filtrado Low Pass
    accChart->addSeries(seriesPitch);
    accChart->addSeries(seriesDynSp); // Agregamos Setpoint Dinámico

    accChart->legend()->setVisible(true);
    accChart->legend()->setAlignment(Qt::AlignBottom);

    //    y las de giroscopio al gyroChart
    // El CSV trae gyro_y, usaremos la serie "seriesGx" (o renombramos variables, pero para minimizar cambios usamos seriesGx como principal)
    // Corrección: El código anterior usaba seriesGy para gyro_y. Mantengamos eso.
    seriesGy->setName("Giroscopio Y");
    seriesGy->setColor(Qt::blue);

    seriesGyroF = new QLineSeries(); seriesGyroF->setName("Giro Filtrado Y");
    seriesGyroF->setColor(Qt::cyan);

    // gyroChart->addSeries(seriesGx);
    gyroChart->addSeries(seriesGy);
    gyroChart->addSeries(seriesGyroF); // Agregamos Gyro Filtrado
    // gyroChart->addSeries(seriesGz);
    gyroChart->legend()->setVisible(true);
    gyroChart->legend()->setAlignment(Qt::AlignBottom);

    // 5) Crea ejes para cada chart
    accAxisX = new QValueAxis(); accAxisX->setTitleText("Tiempo (s)");
    accAxisY = new QValueAxis(); accAxisY->setTitleText("Valor");
    accAxisX->setLabelFormat("%.1f");
    accAxisY->setRange(-2.0, 2.0);         // ajusta a g si muestras en g

    gyroAxisX = new QValueAxis(); gyroAxisX->setTitleText("Tiempo (s)");
    gyroAxisY = new QValueAxis(); gyroAxisY->setTitleText("Valor");
    gyroAxisX->setLabelFormat("%.1f");
    gyroAxisY->setRange(-250.0, 250.0);    // ajusta a °/s si muestras en °/s

    // 6) Añade los ejes a cada chart
    accChart->addAxis(accAxisX, Qt::AlignBottom);
    accChart->addAxis(accAxisY, Qt::AlignLeft);

    gyroChart->addAxis(gyroAxisX, Qt::AlignBottom);
    gyroChart->addAxis(gyroAxisY, Qt::AlignLeft);

    // 7) Asocia series a sus ejes
    // Solo asociamos las que agregamos al gráfico
    seriesAx->attachAxis(accAxisX);
    seriesAx->attachAxis(accAxisY);
    seriesRollFilt->attachAxis(accAxisX); // Attach Roll Filt
    seriesRollFilt->attachAxis(accAxisY);
    seriesAccelRollF->attachAxis(accAxisX);
    seriesAccelRollF->attachAxis(accAxisY);
    seriesPitch->attachAxis(accAxisX);
    seriesPitch->attachAxis(accAxisY);
    seriesDynSp->attachAxis(accAxisX);
    seriesDynSp->attachAxis(accAxisY);

    seriesGy->attachAxis(gyroAxisX);
    seriesGy->attachAxis(gyroAxisY);
    seriesGyroF->attachAxis(gyroAxisX);
    seriesGyroF->attachAxis(gyroAxisY);

    // --- TAB 2: PID ---
    pidChartView = ui->pidWidget;
    pidChart = new QChart();
    pidChartView->setChart(pidChart);
    pidChartView->setRenderHint(QPainter::Antialiasing);
    pidChart->setTitle("PID Control");

    seriesP = new QLineSeries(); seriesP->setName("Proporcional"); seriesP->setColor(Qt::red);
    seriesI = new QLineSeries(); seriesI->setName("Integral"); seriesI->setColor(Qt::green);
    seriesD = new QLineSeries(); seriesD->setName("Derivativo"); seriesD->setColor(Qt::blue);
    seriesOutput = new QLineSeries(); seriesOutput->setName("Output"); seriesOutput->setColor(QColor("#e6edf3")); // claro: visible sobre fondo oscuro
    seriesError = new QLineSeries(); seriesError->setName("Error"); seriesError->setColor(Qt::magenta);

    pidChart->addSeries(seriesP);
    pidChart->addSeries(seriesI);
    pidChart->addSeries(seriesD);
    pidChart->addSeries(seriesOutput);
    pidChart->addSeries(seriesError);

    pidAxisX = new QValueAxis(); pidAxisX->setTitleText("Tiempo (s)");
    pidAxisY = new QValueAxis(); pidAxisY->setTitleText("Valor");
    pidChart->addAxis(pidAxisX, Qt::AlignBottom);
    pidChart->addAxis(pidAxisY, Qt::AlignLeft);

    for (auto *s : {seriesP, seriesI, seriesD, seriesOutput, seriesError}) {
        s->attachAxis(pidAxisX);
        s->attachAxis(pidAxisY);
    }
    pidChart->legend()->setVisible(true);
    pidChart->legend()->setAlignment(Qt::AlignBottom);


    // --- TAB 3: MOTORES ---
    motorsChartView = ui->motorsWidget;
    motorsChart = new QChart();
    motorsChartView->setChart(motorsChart);
    motorsChartView->setRenderHint(QPainter::Antialiasing);
    motorsChart->setTitle("Motores & PWM");

    seriesPwmCmd = new QLineSeries(); seriesPwmCmd->setName("PWM Cmd"); seriesPwmCmd->setColor(Qt::darkGreen);
    seriesPwmSat = new QLineSeries(); seriesPwmSat->setName("PWM Sat"); seriesPwmSat->setColor(Qt::darkRed);
    seriesMR = new QLineSeries(); seriesMR->setName("Motor Der"); seriesMR->setColor(Qt::blue);
    seriesML = new QLineSeries(); seriesML->setName("Motor Izq"); seriesML->setColor(Qt::cyan);

    motorsChart->addSeries(seriesPwmCmd);
    motorsChart->addSeries(seriesPwmSat);
    motorsChart->addSeries(seriesMR);
    motorsChart->addSeries(seriesML);

    motorsAxisX = new QValueAxis(); motorsAxisX->setTitleText("Tiempo (s)");
    motorsAxisY = new QValueAxis(); motorsAxisY->setTitleText("Valor");
    motorsChart->addAxis(motorsAxisX, Qt::AlignBottom);
    motorsChart->addAxis(motorsAxisY, Qt::AlignLeft);

    for (auto *s : {seriesPwmCmd, seriesPwmSat, seriesMR, seriesML}) {
        s->attachAxis(motorsAxisX);
        s->attachAxis(motorsAxisY);
    }
    motorsChart->legend()->setVisible(true);
    motorsChart->legend()->setAlignment(Qt::AlignBottom);

    // --- TAB 4: SISTEMA ---
    systemChartView = ui->systemWidget;
    systemChart = new QChart();
    systemChartView->setChart(systemChart);
    systemChartView->setRenderHint(QPainter::Antialiasing);
    systemChart->setTitle("Sistema (Timing & Flags)");

    seriesDt = new QLineSeries(); seriesDt->setName("Delta Time (us)"); seriesDt->setColor(Qt::darkBlue);
    seriesSatFlag = new QLineSeries(); seriesSatFlag->setName("Sat Flag"); seriesSatFlag->setColor(Qt::red);
    seriesDtCtrl = new QLineSeries(); seriesDtCtrl->setName("DT Control (us)"); seriesDtCtrl->setColor(Qt::darkGreen);

    systemChart->addSeries(seriesDt);
    systemChart->addSeries(seriesDtCtrl);
    systemChart->addSeries(seriesSatFlag);

    systemAxisX = new QValueAxis(); systemAxisX->setTitleText("Tiempo (s)");
    systemAxisY = new QValueAxis(); systemAxisY->setTitleText("Valor");
    systemChart->addAxis(systemAxisX, Qt::AlignBottom);
    systemChart->addAxis(systemAxisY, Qt::AlignLeft);

    seriesDt->attachAxis(systemAxisX);
    seriesDt->attachAxis(systemAxisY);
    seriesDtCtrl->attachAxis(systemAxisX);
    seriesDtCtrl->attachAxis(systemAxisY);
    seriesSatFlag->attachAxis(systemAxisX);
    seriesSatFlag->attachAxis(systemAxisY);

    systemChart->legend()->setVisible(true);
    systemChart->legend()->setAlignment(Qt::AlignBottom);

    // --- TAB 5: SENSORES RAW (NUEVO) ---
    QWidget *tabRawSensors = new QWidget();
    QVBoxLayout *layoutRaw = new QVBoxLayout(tabRawSensors);

    // Chart Acelerómetro Raw
    rawSensorsAccChartView = new QChartView();
    rawSensorsAccChart = new QChart();
    rawSensorsAccChartView->setChart(rawSensorsAccChart);
    rawSensorsAccChartView->setRenderHint(QPainter::Antialiasing);
    rawSensorsAccChart->setTitle("Acelerómetro Raw (X, Y, Z)");

    seriesRawAx = new QLineSeries(); seriesRawAx->setName("Ax"); seriesRawAx->setColor(Qt::red);
    seriesRawAy = new QLineSeries(); seriesRawAy->setName("Ay"); seriesRawAy->setColor(Qt::green);
    seriesRawAz = new QLineSeries(); seriesRawAz->setName("Az"); seriesRawAz->setColor(Qt::blue);

    rawSensorsAccChart->addSeries(seriesRawAx);
    rawSensorsAccChart->addSeries(seriesRawAy);
    rawSensorsAccChart->addSeries(seriesRawAz);

    rawSensorsAccAxisX = new QValueAxis(); rawSensorsAccAxisX->setTitleText("Tiempo (s)");
    rawSensorsAccAxisY = new QValueAxis(); rawSensorsAccAxisY->setTitleText("m/s²");
    rawSensorsAccChart->addAxis(rawSensorsAccAxisX, Qt::AlignBottom);
    rawSensorsAccChart->addAxis(rawSensorsAccAxisY, Qt::AlignLeft);

    for (auto *s : {seriesRawAx, seriesRawAy, seriesRawAz}) {
        s->attachAxis(rawSensorsAccAxisX);
        s->attachAxis(rawSensorsAccAxisY);
    }

    // Chart Giroscopio Raw
    rawSensorsGyroChartView = new QChartView();
    rawSensorsGyroChart = new QChart();
    rawSensorsGyroChartView->setChart(rawSensorsGyroChart);
    rawSensorsGyroChartView->setRenderHint(QPainter::Antialiasing);
    rawSensorsGyroChart->setTitle("Giroscopio Raw (X, Y, Z)");

    seriesRawGx = new QLineSeries(); seriesRawGx->setName("Gx"); seriesRawGx->setColor(Qt::red);
    seriesRawGy = new QLineSeries(); seriesRawGy->setName("Gy"); seriesRawGy->setColor(Qt::green);
    seriesRawGz = new QLineSeries(); seriesRawGz->setName("Gz"); seriesRawGz->setColor(Qt::blue);

    rawSensorsGyroChart->addSeries(seriesRawGx);
    rawSensorsGyroChart->addSeries(seriesRawGy);
    rawSensorsGyroChart->addSeries(seriesRawGz);

    rawSensorsGyroAxisX = new QValueAxis(); rawSensorsGyroAxisX->setTitleText("Tiempo (s)");
    rawSensorsGyroAxisY = new QValueAxis(); rawSensorsGyroAxisY->setTitleText("deg/s");
    rawSensorsGyroChart->addAxis(rawSensorsGyroAxisX, Qt::AlignBottom);
    rawSensorsGyroChart->addAxis(rawSensorsGyroAxisY, Qt::AlignLeft);

    for (auto *s : {seriesRawGx, seriesRawGy, seriesRawGz}) {
        s->attachAxis(rawSensorsGyroAxisX);
        s->attachAxis(rawSensorsGyroAxisY);
    }

    // Agregar charts al layout
    layoutRaw->addWidget(rawSensorsAccChartView);
    layoutRaw->addWidget(rawSensorsGyroChartView);

    // Agregar tab al widget principal
    ui->tabWidget_Graficas->addTab(tabRawSensors, "Sensores Raw");

    // 7) Iniciar el tiempo
    elapsedSec = 0;
    mpuPollTimer = new QTimer(this);
    connect(mpuPollTimer, &QTimer::timeout, this, &MainWindow::updatePlot);
    mpuStarted    = false;

    // --- TAB 6: VALORES ADC (NUEVO) ---
    QWidget *tabAdc = new QWidget();
    QVBoxLayout *layoutAdc = new QVBoxLayout(tabAdc);

    // 1) Crear y asociar tu widget para el tab
    adcChartView = new QChartView();
    layoutAdc->addWidget(adcChartView);
    ui->tabWidget_Graficas->addTab(tabAdc, "Valores ADC");

    // 2) CREAR chart + serie
    adcChart  = new QChart();
    adcSeries = new QBarSeries();
    adcBarSet = new QBarSet("ADC");
    *adcBarSet << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0;
    adcSeries->append(adcBarSet);

    // 3) AÑADIR la serie ANTES de cualquier eje
    adcChart->addSeries(adcSeries);

    // 4) CREAR/ANEXAR ejes
    adcAxisX = new QBarCategoryAxis();
    adcAxisX->append(QStringList{"1","2","3","4","5","6","7","8"});
    adcChart->addAxis(adcAxisX, Qt::AlignBottom);
    adcSeries->attachAxis(adcAxisX);

    adcAxisY = new QValueAxis();
    adcAxisY->setRange(0, 4100);
    adcChart->addAxis(adcAxisY, Qt::AlignLeft);
    adcSeries->attachAxis(adcAxisY);

    // 5) FINALIZAR
    adcChart->setTitle("Valores ADC");
    adcChartView->setChart(adcChart);
    adcChartView->setRenderHint(QPainter::Antialiasing);

    // --- TAB 7: SEGUIDOR DE LÍNEA (NUEVO) ---
    QWidget *tabLineFollower = new QWidget();
    QVBoxLayout *layoutLineFollower = new QVBoxLayout(tabLineFollower);

    // Chart Line Follower PID
    lineFollowerPidChartView = new QChartView();
    lineFollowerPidChart = new QChart();
    lineFollowerPidChartView->setChart(lineFollowerPidChart);
    lineFollowerPidChartView->setRenderHint(QPainter::Antialiasing);
    lineFollowerPidChart->setTitle("Controlador PID Seguidor de Línea");

    seriesLineError = new QLineSeries(); seriesLineError->setName("Error"); seriesLineError->setColor(Qt::magenta);
    seriesLineP = new QLineSeries(); seriesLineP->setName("Proporcional"); seriesLineP->setColor(Qt::red);
    seriesLineI = new QLineSeries(); seriesLineI->setName("Integral"); seriesLineI->setColor(Qt::green);
    seriesLineD = new QLineSeries(); seriesLineD->setName("Derivativo"); seriesLineD->setColor(Qt::blue);
    seriesLineSteering = new QLineSeries(); seriesLineSteering->setName("Ajuste Giro"); seriesLineSteering->setColor(QColor("#e6edf3")); // claro: visible sobre fondo oscuro

    lineFollowerPidChart->addSeries(seriesLineError);
    lineFollowerPidChart->addSeries(seriesLineP);
    lineFollowerPidChart->addSeries(seriesLineI);
    lineFollowerPidChart->addSeries(seriesLineD);
    lineFollowerPidChart->addSeries(seriesLineSteering);

    lineFollowerPidAxisX = new QValueAxis(); lineFollowerPidAxisX->setTitleText("Tiempo (s)");
    lineFollowerPidAxisY = new QValueAxis(); lineFollowerPidAxisY->setTitleText("Valor");
    lineFollowerPidChart->addAxis(lineFollowerPidAxisX, Qt::AlignBottom);
    lineFollowerPidChart->addAxis(lineFollowerPidAxisY, Qt::AlignLeft);

    for (auto *s : {seriesLineError, seriesLineP, seriesLineI, seriesLineD, seriesLineSteering}) {
        s->attachAxis(lineFollowerPidAxisX);
        s->attachAxis(lineFollowerPidAxisY);
    }

    lineFollowerPidChart->legend()->setVisible(true);
    lineFollowerPidChart->legend()->setAlignment(Qt::AlignBottom);

    // Chart Line Follower ADCs
    lineFollowerAdcChartView = new QChartView();
    lineFollowerAdcChart = new QChart();
    lineFollowerAdcChartView->setChart(lineFollowerAdcChart);
    lineFollowerAdcChartView->setRenderHint(QPainter::Antialiasing);
    lineFollowerAdcChart->setTitle("Sensores ADC Seguidor de Línea (Extremo Der - Centro Der - Centro Izq - Extremo Izq)");

    seriesLineAdc1 = new QLineSeries(); seriesLineAdc1->setName("ADC 1 (Der Ext)"); seriesLineAdc1->setColor(Qt::red);
    seriesLineAdc2 = new QLineSeries(); seriesLineAdc2->setName("ADC 2 (Der Cen)"); seriesLineAdc2->setColor(Qt::green);
    seriesLineAdc3 = new QLineSeries(); seriesLineAdc3->setName("ADC 3 (Izq Cen)"); seriesLineAdc3->setColor(Qt::blue);
    seriesLineAdc4 = new QLineSeries(); seriesLineAdc4->setName("ADC 4 (Izq Ext)"); seriesLineAdc4->setColor(Qt::magenta);

    lineFollowerAdcChart->addSeries(seriesLineAdc1);
    lineFollowerAdcChart->addSeries(seriesLineAdc2);
    lineFollowerAdcChart->addSeries(seriesLineAdc3);
    lineFollowerAdcChart->addSeries(seriesLineAdc4);

    lineFollowerAdcAxisX = new QValueAxis(); lineFollowerAdcAxisX->setTitleText("Tiempo (s)");
    lineFollowerAdcAxisY = new QValueAxis(); lineFollowerAdcAxisY->setTitleText("ADC Value");
    lineFollowerAdcAxisY->setRange(0, 4100);
    lineFollowerAdcChart->addAxis(lineFollowerAdcAxisX, Qt::AlignBottom);
    lineFollowerAdcChart->addAxis(lineFollowerAdcAxisY, Qt::AlignLeft);

    for (auto *s : {seriesLineAdc1, seriesLineAdc2, seriesLineAdc3, seriesLineAdc4}) {
        s->attachAxis(lineFollowerAdcAxisX);
        s->attachAxis(lineFollowerAdcAxisY);
    }

    lineFollowerAdcChart->legend()->setVisible(true);
    lineFollowerAdcChart->legend()->setAlignment(Qt::AlignBottom);

    layoutLineFollower->addWidget(lineFollowerPidChartView);
    layoutLineFollower->addWidget(lineFollowerAdcChartView);

    ui->tabWidget_Graficas->addTab(tabLineFollower, "Seguidor de Línea");

    // --- TAB 8: ODOMETRIA WIFI (Nuevo) ---
    // Alimentado por el paquete WIFI_ODOM_DATA=0xDC, que el firmware empuja solo
    // (push, no hay que pedirlo) cada WIFI_ODOM_PERIOD_MS (500ms por defecto) mientras
    // haya conexión WiFi activa. Mapa navegable (rueda=zoom centrado en el cursor,
    // arrastre=paneo, doble click=reset — ver OdomChartView) con la "cinta" (tramos
    // donde se detectó la línea) dibujada como segmentos negros gruesos, y
    // anotaciones de texto en los puntos donde se perdió la línea o se detectó un
    // objeto.
    QWidget *tabOdom = new QWidget();
    QVBoxLayout *layoutOdom = new QVBoxLayout(tabOdom);

    lblOdomInfo = new QLabel("Odometría: esperando datos por WiFi...");
    lblOdomInfo->setAlignment(Qt::AlignCenter);
    layoutOdom->addWidget(lblOdomInfo);

    QHBoxLayout *layoutOdomButtons = new QHBoxLayout();
    QPushButton *btnOdomReset  = new QPushButton("Reset vista");
    QPushButton *btnOdomClear  = new QPushButton("Borrar mapa");
    QPushButton *btnOdomFollow = new QPushButton("Seguir robot");
    QPushButton *btnOdomMeasure = new QPushButton("Medir");
    QPushButton *btnOdomPng    = new QPushButton("Guardar PNG");
    btnOdomFollow->setCheckable(true);
    btnOdomMeasure->setCheckable(true);
    btnOdomReset->setToolTip("Vuelve la vista al encuadre automático (también: doble click en el mapa)");
    btnOdomFollow->setToolTip("La vista se recentra sola en el robot a cada muestra (paneo automático)");
    btnOdomMeasure->setToolTip("Medición estilo CAD: click en dos puntos del mapa para medir la distancia en metros");
    btnOdomPng->setToolTip("Guarda una captura del mapa como imagen PNG");
    layoutOdomButtons->addWidget(btnOdomReset);
    layoutOdomButtons->addWidget(btnOdomClear);
    layoutOdomButtons->addWidget(btnOdomFollow);
    layoutOdomButtons->addWidget(btnOdomMeasure);
    layoutOdomButtons->addWidget(btnOdomPng);
    layoutOdomButtons->addStretch();
    lblOdomCursor = new QLabel("( --.-- , --.-- ) m");
    {
        QFont cf = lblOdomCursor->font();
        cf.setFamily("Courier");
        cf.setBold(true);
        lblOdomCursor->setFont(cf);
    }
    layoutOdomButtons->addWidget(lblOdomCursor);
    layoutOdom->addLayout(layoutOdomButtons);

    odomChartView = new OdomChartView();
    odomChart = new QChart();
    odomChartView->setChart(odomChart);
    odomChartView->setRenderHint(QPainter::Antialiasing);

    // Estética: mapa oscuro (mismo clima que la Vista 3D), sin título — las
    // instrucciones de navegación van como tooltip para no gastar espacio.
    odomChartView->setToolTip("Rueda: zoom (centrado en el cursor) · Arrastre: paneo · Doble click: reset de vista");
    odomChart->setBackgroundBrush(QBrush(QColor(10, 14, 8)));
    odomChart->setBackgroundRoundness(0);
    odomChart->setPlotAreaBackgroundBrush(QBrush(QColor(14, 20, 11)));
    odomChart->setPlotAreaBackgroundVisible(true);
    odomChart->setMargins(QMargins(4, 4, 4, 4));

    odomTrailSeries = new QLineSeries();
    odomTrailSeries->setName("Trayectoria");
    QPen trailPen(QColor("#8a97a8"));
    trailPen.setWidth(2);
    odomTrailSeries->setPen(trailPen);

    odomLostMarkers = new QScatterSeries();
    odomLostMarkers->setName("Línea perdida");
    odomLostMarkers->setColor(Qt::red);
    odomLostMarkers->setMarkerSize(10.0);

    odomObjMarkers = new QScatterSeries();
    odomObjMarkers->setName("Objeto detectado");
    odomObjMarkers->setColor(QColor(255, 140, 0)); // naranja
    odomObjMarkers->setMarkerSize(10.0);

    odomCurrentSeries = new QScatterSeries();
    odomCurrentSeries->setName("Posición actual");
    odomCurrentSeries->setColor(QColor("#3d8bfd"));
    odomCurrentSeries->setMarkerSize(12.0);

    // Barrera "viva": segmento rojo grueso perpendicular al rumbo, dibujado frente
    // al robot mientras los sensores de objeto (ADC5/6/8) están viendo algo.
    odomBarrierSeries = new QLineSeries();
    odomBarrierSeries->setName("Objeto adelante");
    QPen barrierPen(QColor("#e5484d"));
    barrierPen.setWidth(5);
    barrierPen.setCapStyle(Qt::RoundCap);
    odomBarrierSeries->setPen(barrierPen);

    // Rastro persistente: puntito donde se estimó el obstáculo en cada muestra
    // (frontal por ADC5/6/8, lateral izquierdo por ADC7) — al bordear el objeto
    // va quedando dibujado su contorno aproximado.
    odomObstaclePts = new QScatterSeries();
    odomObstaclePts->setName("Obstáculo visto");
    odomObstaclePts->setColor(QColor(229, 72, 77, 140)); // rojo semitransparente
    odomObstaclePts->setBorderColor(Qt::transparent);
    odomObstaclePts->setMarkerSize(6.0);

    // Cruz del origen (0,0): referencia fija sutil, fuera de la leyenda.
    odomOriginH = new QLineSeries();
    odomOriginV = new QLineSeries();
    {
        QPen originPen(QColor(90, 110, 90));
        originPen.setWidth(1);
        odomOriginH->setPen(originPen);
        odomOriginV->setPen(originPen);
        odomOriginH->append(-0.06, 0.0); odomOriginH->append(0.06, 0.0);
        odomOriginV->append(0.0, -0.06); odomOriginV->append(0.0, 0.06);
    }

    // Línea de medición (modo "Medir"): amarilla punteada, con la distancia
    // flotando en el punto medio (ver odomMeasureText más abajo).
    odomMeasureSeries = new QLineSeries();
    {
        QPen measurePen(QColor(255, 210, 70));
        measurePen.setWidth(2);
        measurePen.setStyle(Qt::DashLine);
        odomMeasureSeries->setPen(measurePen);
    }

    odomChart->addSeries(odomOriginH);
    odomChart->addSeries(odomOriginV);
    odomChart->addSeries(odomTrailSeries);
    odomChart->addSeries(odomLostMarkers);
    odomChart->addSeries(odomObjMarkers);
    odomChart->addSeries(odomObstaclePts);
    odomChart->addSeries(odomBarrierSeries);
    odomChart->addSeries(odomCurrentSeries);
    odomChart->addSeries(odomMeasureSeries);

    // Mapa limpio: sin título de eje, sin líneas de referencia (grilla) ni línea/
    // números de eje — solo queda dibujado el recorrido en sí. Los ejes se
    // mantienen como objetos (siguen definiendo la escala/rango) pero invisibles.
    odomAxisX = new QValueAxis();
    odomAxisY = new QValueAxis();
    odomAxisX->setTitleText(""); odomAxisY->setTitleText("");
    // Grilla sutil: da percepción de escala sin números de eje (la escala exacta
    // la muestra la barra de abajo a la izquierda).
    QPen gridPen(QColor(28, 40, 24));
    gridPen.setWidth(1);
    odomAxisX->setGridLineVisible(true);  odomAxisY->setGridLineVisible(true);
    odomAxisX->setGridLinePen(gridPen);   odomAxisY->setGridLinePen(gridPen);
    odomAxisX->setMinorGridLineVisible(false); odomAxisY->setMinorGridLineVisible(false);
    odomAxisX->setLabelsVisible(false); odomAxisY->setLabelsVisible(false);
    odomAxisX->setLineVisible(false); odomAxisY->setLineVisible(false);
    odomChart->addAxis(odomAxisX, Qt::AlignBottom);
    odomChart->addAxis(odomAxisY, Qt::AlignLeft);
    odomAxisX->setRange(odomMinX, odomMaxX);
    odomAxisY->setRange(odomMinY, odomMaxY);

    for (auto *s : {static_cast<QAbstractSeries*>(odomOriginH),
                     static_cast<QAbstractSeries*>(odomOriginV),
                     static_cast<QAbstractSeries*>(odomTrailSeries),
                     static_cast<QAbstractSeries*>(odomLostMarkers),
                     static_cast<QAbstractSeries*>(odomObjMarkers),
                     static_cast<QAbstractSeries*>(odomObstaclePts),
                     static_cast<QAbstractSeries*>(odomBarrierSeries),
                     static_cast<QAbstractSeries*>(odomCurrentSeries),
                     static_cast<QAbstractSeries*>(odomMeasureSeries)}) {
        s->attachAxis(odomAxisX);
        s->attachAxis(odomAxisY);
    }

    odomChart->legend()->setVisible(true);
    odomChart->legend()->setAlignment(Qt::AlignBottom);
    odomChart->legend()->setLabelColor(QColor(190, 205, 185));
    // Series auxiliares (origen, medición) fuera de la leyenda.
    for (auto *aux : {static_cast<QAbstractSeries*>(odomOriginH),
                       static_cast<QAbstractSeries*>(odomOriginV),
                       static_cast<QAbstractSeries*>(odomMeasureSeries)}) {
        const auto markers = odomChart->legend()->markers(aux);
        if (!markers.isEmpty()) markers.first()->setVisible(false);
    }

    // Flecha de rumbo: dos QGraphicsItems sueltos (no forman parte de ninguna
    // serie) que indican hacia dónde apunta el robot en la posición actual.
    odomHeadingLine = new QGraphicsLineItem();
    odomHeadingLine->setPen(QPen(QColor("#3d8bfd"), 2));
    odomChart->scene()->addItem(odomHeadingLine);

    odomHeadingArrowHead = new QGraphicsPolygonItem();
    odomHeadingArrowHead->setPen(QPen(QColor("#3d8bfd")));
    odomHeadingArrowHead->setBrush(QBrush(QColor("#3d8bfd")));
    odomChart->scene()->addItem(odomHeadingArrowHead);

    // Barra de escala (abajo a la izquierda): como los ejes no tienen números,
    // es la referencia de cuántos metros representa lo que se ve. Se recalcula
    // a un largo "redondo" (0.1/0.2/0.5/1/2/5 m) en cada zoom/paneo/autoescala.
    odomScaleLine = new QGraphicsLineItem();
    odomScaleLine->setPen(QPen(QColor(190, 205, 185), 2));
    odomChart->scene()->addItem(odomScaleLine);
    odomScaleText = new QGraphicsSimpleTextItem();
    odomScaleText->setBrush(QBrush(QColor(190, 205, 185)));
    {
        QFont sf = odomScaleText->font();
        sf.setPointSize(8);
        sf.setBold(true);
        odomScaleText->setFont(sf);
    }
    odomChart->scene()->addItem(odomScaleText);

    // Texto flotante de la medición (distancia en metros sobre la línea amarilla).
    odomMeasureText = new QGraphicsSimpleTextItem();
    odomMeasureText->setBrush(QBrush(QColor(255, 210, 70)));
    {
        QFont mf = odomMeasureText->font();
        mf.setPointSize(9);
        mf.setBold(true);
        odomMeasureText->setFont(mf);
    }
    odomChart->scene()->addItem(odomMeasureText);

    // Reposicionar las anotaciones de texto y la flecha de rumbo (QGraphicsItems
    // sueltos, fuera de las series) cada vez que el usuario hace zoom/paneo.
    connect(odomChartView, &OdomChartView::viewChanged, this, &MainWindow::repositionOdomAnnotations);
    connect(odomChartView, &OdomChartView::hoverAt, this, &MainWindow::onOdomHover);
    connect(odomChartView, &OdomChartView::measureClick, this, &MainWindow::onOdomMeasureClick);
    connect(btnOdomReset, &QPushButton::clicked, this, [this]() {
        odomChart->zoomReset();
        repositionOdomAnnotations();
    });
    connect(btnOdomClear, &QPushButton::clicked, this, &MainWindow::clearOdomMap);
    connect(btnOdomFollow, &QPushButton::toggled, this, [this](bool on) {
        odomFollowRobot = on;
    });
    connect(btnOdomMeasure, &QPushButton::toggled, this, [this](bool on) {
        odomChartView->setMeasureMode(on);
        odomMeasureHasP1 = false;
        odomMeasureDone  = false;
        odomMeasureSeries->clear();
        if (odomMeasureText) odomMeasureText->setText("");
    });
    connect(btnOdomPng, &QPushButton::clicked, this, &MainWindow::saveOdomMapPng);

    layoutOdom->addWidget(odomChartView);
    ui->tabWidget_Graficas->addTab(tabOdom, "Odometría");

    // --- TAB MPU CRUDO: valores numéricos de acelerómetro y giroscopio ---
    {
        auto makeValueLabel = [](QWidget *parent) {
            auto *lbl = new QLabel("---", parent);
            QFont f = lbl->font();
            f.setPointSize(28);
            f.setBold(true);
            f.setFamily("Courier");
            lbl->setFont(f);
            lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            lbl->setMinimumWidth(200);
            return lbl;
        };
        auto makeTitleLabel = [](const QString &text, QWidget *parent) {
            auto *lbl = new QLabel(text, parent);
            QFont f = lbl->font();
            f.setPointSize(14);
            f.setBold(false);
            lbl->setFont(f);
            lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            return lbl;
        };

        QWidget *tabMpu = new QWidget();
        QHBoxLayout *hMain = new QHBoxLayout(tabMpu);
        hMain->setContentsMargins(30, 30, 30, 30);
        hMain->setSpacing(40);

        // --- Bloque Acelerómetro ---
        auto *grpAcc = new QGroupBox("ACELERÓMETRO (m/s²)", tabMpu);
        auto *gridAcc = new QGridLayout(grpAcc);
        gridAcc->setSpacing(16);

        lblRawAx = makeValueLabel(grpAcc);
        lblRawAy = makeValueLabel(grpAcc);
        lblRawAz = makeValueLabel(grpAcc);
        gridAcc->addWidget(makeTitleLabel("Ax", grpAcc), 0, 0);
        gridAcc->addWidget(lblRawAx,                     0, 1);
        gridAcc->addWidget(makeTitleLabel("Ay", grpAcc), 1, 0);
        gridAcc->addWidget(lblRawAy,                     1, 1);
        gridAcc->addWidget(makeTitleLabel("Az", grpAcc), 2, 0);
        gridAcc->addWidget(lblRawAz,                     2, 1);
        gridAcc->setRowStretch(3, 1);

        // --- Bloque Giroscopio ---
        auto *grpGyro = new QGroupBox("GIROSCOPIO (°/s)", tabMpu);
        auto *gridGyro = new QGridLayout(grpGyro);
        gridGyro->setSpacing(16);

        lblRawGx = makeValueLabel(grpGyro);
        lblRawGy = makeValueLabel(grpGyro);
        lblRawGz = makeValueLabel(grpGyro);
        gridGyro->addWidget(makeTitleLabel("Gx", grpGyro), 0, 0);
        gridGyro->addWidget(lblRawGx,                      0, 1);
        gridGyro->addWidget(makeTitleLabel("Gy", grpGyro), 1, 0);
        gridGyro->addWidget(lblRawGy,                      1, 1);
        gridGyro->addWidget(makeTitleLabel("Gz", grpGyro), 2, 0);
        gridGyro->addWidget(lblRawGz,                      2, 1);
        gridGyro->setRowStretch(3, 1);

        hMain->addWidget(grpAcc);
        hMain->addWidget(grpGyro);

        ui->tabWidget_Graficas->addTab(tabMpu, "MPU Crudo");
    }

    // --- TAB Vista 3D ---
    QWidget *tab3D = new QWidget();
    QVBoxLayout *layout3D = new QVBoxLayout(tab3D);
    layout3D->setContentsMargins(0, 0, 0, 0);
    robotViewer3D = new RobotViewer3D(tab3D);
    layout3D->addWidget(robotViewer3D);
    ui->tabWidget_Graficas->addTab(tab3D, "Vista 3D");

    // --- Estilo unificado de todos los gráficos (colores del tema) ---
    adcBarSet->setColor(QColor("#3d8bfd"));
    for (QChart *c : {accChart, gyroChart, pidChart, motorsChart, systemChart,
                      rawSensorsAccChart, rawSensorsGyroChart, adcChart,
                      lineFollowerPidChart, lineFollowerAdcChart, odomChart}) {
        styleChart(c);
    }

    // --- Orden final de pestañas: operación primero, diagnóstico después ---
    // (las pestañas se crean en distintos lugares — .ui y código — así que el
    // orden se fija al final, buscándolas por título)
    {
        const QStringList tabOrder = {
            "Vista 3D", "IMU · Ángulos", "PID Balance", "Motores",
            "Seguidor de Línea", "Odometría",
            "Sensores Raw", "Valores ADC", "MPU Crudo", "Sistema"
        };
        QTabBar *bar = ui->tabWidget_Graficas->tabBar();
        for (int target = 0; target < tabOrder.size() && target < bar->count(); ++target) {
            for (int i = target; i < bar->count(); ++i) {
                if (ui->tabWidget_Graficas->tabText(i) == tabOrder[target]) {
                    bar->moveTab(i, target);
                    break;
                }
            }
        }
    }
    ui->tabWidget_Graficas->setCurrentIndex(0);

    // --- UDP abierto por defecto al iniciar (2026-07-10) ---
    // Mismo camino que el botón "Abrir UDP" (bind al puerto local por defecto,
    // 30010, watchdog + ping alive incluidos). Diferido con singleShot(0) para
    // que corra con la ventana ya construida; solo si el puerto de la UI es
    // válido y el socket sigue cerrado (no pisa nada si el usuario fue más rápido).
    QTimer::singleShot(0, this, [this]() {
        bool okPort = false;
        const int p = ui->lineEdit_LOCALPORT->text().toInt(&okPort, 10);
        if (okPort && p > 0 && p <= 65535 &&
            UdpSocket1->state() == QAbstractSocket::UnconnectedState) {
            on_pushButton_OPENUDP_clicked();
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSerialPorts(){
    const SettingsDialog::Settings p = settingPorts->settings();

    // Validar si el puerto es válido o está vacío
    if(p.name.isEmpty()) {
        QMessageBox::warning(this, tr("Advertencia"), tr("No se ha seleccionado ningún puerto. Por favor ve a Device -> Scan Ports y selecciona uno."));
        // Intentar abrir el diálogo automáticamente si no hay configuración
        settingPorts->show();
        return;
    }

    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    serial->open(QSerialPort::ReadWrite);
    if (serial->isOpen()){
        ui->pushButton_BALANCE->setEnabled(true);
        ui->pushButton_FOLLOW_LINE->setEnabled(true);
        ui->pushButton_SEND->setEnabled(true);
        ui->actionDisconnect_Device->setEnabled(true);
        estadoSerial->setStyleSheet("QLabel { color: #58a6ff; }");
        estadoSerial->setText(tr("Connected to %1 : %2, %3, %4, %5, %6, %7")
                                  .arg(p.name)
                                  .arg(p.stringBaudRate)
                                  .arg(p.stringDataBits)
                                  .arg(p.stringParity)
                                  .arg(p.stringStopBits)
                                  .arg(p.stringFlowControl)
                                  .arg(p.fabricante)
                              );
    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());
    }
}

void MainWindow::closeSerialPorts(){
    ui->actionDisconnect_Device->setEnabled(false);
    estadoSerial->setText("Desconectado ......");
    if (serial->isOpen())
        serial->close();
}

void MainWindow::dataRecived(){
    unsigned char *incomingBuffer;
    int count;

    count = serial->bytesAvailable();

    if(count<=0)
        return;

    incomingBuffer = new unsigned char[count];

    serial->read((char *)incomingBuffer,count);

    // --- PROCESAMIENTO CSV ---
    csvBuffer.append((char *)incomingBuffer, count);
    while (csvBuffer.contains('\n')) {
        int idx = csvBuffer.indexOf('\n');
        QByteArray line = csvBuffer.left(idx).trimmed(); // Extraer línea sin \n
        csvBuffer = csvBuffer.mid(idx + 1);              // Avanzar buffer

        if (!line.isEmpty()) {
            processCsvLine(line);
        }
    }
    // Limpieza de seguridad si el buffer crece demasiado (por ej. si solo llega binario)
    if (csvBuffer.size() > 4096) csvBuffer.clear();
    // -------------------------

    QString str="";

    for(int i=0; i<count; i++){
        if(isalnum(incomingBuffer[i]))
            str = str + QString("%1").arg((char)incomingBuffer[i]);
        else
            str = str +"{" + QString("%1").arg(incomingBuffer[i],2,16,QChar('0')) + "}";
    }
    ui->textEdit_RAW->append("MBED-->SERIAL-->PC (" + str + ")");

    //Cada vez que se recibe un dato reinicio el timeOut
    rxData.timeOut=6;

    for(int i=0;i<count; i++){
        switch (estadoProtocolo) {
        case START:
            if (incomingBuffer[i]=='U'){
                estadoProtocolo=HEADER_1;
            }
            break;
        case HEADER_1:
            if (incomingBuffer[i]=='N')
                estadoProtocolo=HEADER_2;
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case HEADER_2:
            if (incomingBuffer[i]=='E')
                estadoProtocolo=HEADER_3;
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case HEADER_3:
            if (incomingBuffer[i]=='R')
                estadoProtocolo=NBYTES;
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case NBYTES:
            rxData.nBytes=incomingBuffer[i];
            estadoProtocolo=TOKEN;
            break;
        case TOKEN:
            if (incomingBuffer[i]==':'){
                estadoProtocolo=PAYLOAD;
                rxData.cheksum='U'^'N'^'E'^'R'^ rxData.nBytes^':';
                rxData.payLoad[0]=rxData.nBytes;
                rxData.index=1;
            }
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case PAYLOAD:
            if (rxData.nBytes>1){
                rxData.payLoad[rxData.index++]=incomingBuffer[i];
                rxData.cheksum^=incomingBuffer[i];
            }
            rxData.nBytes--;
            if(rxData.nBytes==0){
                estadoProtocolo=START;
                if(rxData.cheksum==incomingBuffer[i]){
                    decodeData(&rxData.payLoad[0], SERIE);
                }else{
                    ui->textEdit_RAW->append("Chk Calculado ** " +QString().number(rxData.cheksum,16) + " **" );
                    ui->textEdit_RAW->append("Chk recibido ** " +QString().number(incomingBuffer[i],16) + " **" );

                }
            }
            break;
        default:
            estadoProtocolo=START;
            break;
        }
    }
    delete [] incomingBuffer;

}

void MainWindow::timeOut(){
    if(rxData.timeOut){
        rxData.timeOut--;
        if(!rxData.timeOut){
            estadoProtocolo=START;
        }
    }
}

void MainWindow::on_pushButton_BALANCE_clicked()
{
    int idx = ui->comboBox_CMD->findData(BALANCE);
    if (idx >= 0) ui->comboBox_CMD->setCurrentIndex(idx);

    if (serial->isOpen()) {
        sendDataSerial();
    } else if (UdpSocket1->state() == QAbstractSocket::BoundState
               && !clientAddress.isNull() && puertoremoto > 0) {
        sendDataUDP();
    } else {
        ui->textEdit_PROCCES->append("BALANCE: Sin conexión disponible.");
    }
}

void MainWindow::on_pushButton_FOLLOW_LINE_clicked()
{
    int idx = ui->comboBox_CMD->findData(ACTIVATE_LINE_FOLLOWING);
    if (idx >= 0) ui->comboBox_CMD->setCurrentIndex(idx);

    if (serial->isOpen()) {
        sendDataSerial();
    } else if (UdpSocket1->state() == QAbstractSocket::BoundState
               && !clientAddress.isNull() && puertoremoto > 0) {
        sendDataUDP();
    } else {
        ui->textEdit_PROCCES->append("FOLLOW LINE: Sin conexión disponible.");
    }
}

void MainWindow::on_pushButton_OPENUDP_clicked()
{
    bool ok = false;
    const int localPort = ui->lineEdit_LOCALPORT->text().toInt(&ok, 10);
    if (!ok || localPort <= 0 || localPort > 65535) {
        QMessageBox::information(this, tr("SERVER PORT"), tr("ERROR: invalid local PORT."));
        return;
    }

    // Toggle: si ya está abierto, cerramos y salimos
    if (UdpSocket1->state() != QAbstractSocket::UnconnectedState) {
        UdpSocket1->close();
        ui->pushButton_OPENUDP->setText("Abrir UDP");
        setToggleState(ui->pushButton_OPENUDP, "off");
        setToggleState(ui->pushButton_BALANCE, "off");
        setToggleState(ui->pushButton_FOLLOW_LINE, "off");
        isBalanceActive = false;
        isFollowLineActive = false;
        ui->pushButton_UDP->setEnabled(false);
        ui->pushButton_BALANCE->setEnabled(false);
        ui->pushButton_FOLLOW_LINE->setEnabled(false);
        ui->lineEdit_IP_REMOTA->clear();
        ui->lineEdit_DEVICEPORT->clear();

        udpWatchdogTimer->stop();
        udpEverReceivedData = false;
        ui->spinBox_SETPOINT->setValue(0.0);

        alivePingTimer->stop();
        healthDashboard->setConnectionOpen(false);

        return;
    }

    // Limpio cualquier estado previo y hago bind robusto
    UdpSocket1->abort();
    const bool okBind = UdpSocket1->bind(
        QHostAddress::AnyIPv4,
        static_cast<quint16>(localPort),
        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
        );

    if (!okBind) {
        QMessageBox::critical(this, tr("SERVER PORT"),
                              tr("Can't bind UDP port %1: %2")
                                  .arg(localPort).arg(UdpSocket1->errorString()));
        return;
    }

    ui->pushButton_OPENUDP->setText("Cerrar UDP");
    setToggleState(ui->pushButton_OPENUDP, "on");
    ui->pushButton_UDP->setEnabled(true);
    ui->pushButton_BALANCE->setEnabled(true);
    ui->pushButton_FOLLOW_LINE->setEnabled(true);
    setToggleState(ui->pushButton_BALANCE, "off");
    setToggleState(ui->pushButton_FOLLOW_LINE, "off");

    udpEverReceivedData = false;
    udpWatchdogTimer->start(1000);

    healthDashboard->setConnectionOpen(true);
    alivePingTimer->start(2000);

    // Cargar destino si ya está ingresado en la UI
    if (clientAddress.isNull()) {
        clientAddress.setAddress(ui->lineEdit_IP_REMOTA->text().trimmed());
    }
    if (puertoremoto == 0) {
        bool okp = false;
        puertoremoto = ui->lineEdit_DEVICEPORT->text().toInt(&okp);
        if (!okp) puertoremoto = 0;
    }

    // (Opcional) ping de descubrimiento si ya tengo IP/puerto remoto
    if (!clientAddress.isNull() && puertoremoto > 0) {
        static const char pingByte = 'r';
        UdpSocket1->writeDatagram(&pingByte, 1, clientAddress, static_cast<quint16>(puertoremoto));
    }
}


void MainWindow::OnUdpRxData()
{
    while (UdpSocket1->hasPendingDatagrams()) {
        udpLastRxTime.start();
        udpEverReceivedData = true;
        healthDashboard->onPacketReceived();

        QNetworkDatagram dgram = UdpSocket1->receiveDatagram();
        const QByteArray payload = dgram.data();
        RemoteAddress = dgram.senderAddress();
        RemotePort    = dgram.senderPort();

        // Aprender/actualizar IP/puerto destino (para respuestas)
        clientAddress = RemoteAddress;
        puertoremoto  = RemotePort;
        ui->pushButton_BALANCE->setEnabled(true);
        ui->pushButton_FOLLOW_LINE->setEnabled(true);

        // Refrescar UI con lo recibido
        ui->lineEdit_IP_REMOTA->setText(RemoteAddress.toString());
        ui->lineEdit_DEVICEPORT->setText(QString::number(RemotePort));

        // Log amigable
        QString str;
        str.reserve(payload.size() * 4);
        for (int i = 0; i < payload.size(); ++i) {
            unsigned char b = static_cast<unsigned char>(payload.at(i));
            if (isalnum(b)) str += QChar(b);
            else str += "{" + QString("%1").arg(b, 2, 16, QChar('0')) + "}";
        }
        ui->textEdit_RAW->append("MBED-->UDP-->PC (" + str + ")");
        ui->textEdit_RAW->append(" adr " + RemoteAddress.toString());

        // ---------- Parser de protocolo UNER ----------
        // Evita OOB: usar índice < payload.size()
        for (int i = 0; i < payload.size(); ++i) {
            unsigned char ch = static_cast<unsigned char>(payload.at(i));
            switch (estadoProtocoloUdp) {
            case START:
                if (ch == 'U') { estadoProtocoloUdp = HEADER_1; rxDataUdp.cheksum = 0; }
                break;
            case HEADER_1:
                if (ch == 'N') estadoProtocoloUdp = HEADER_2;
                else { i--; estadoProtocoloUdp = START; }
                break;
            case HEADER_2:
                if (ch == 'E') estadoProtocoloUdp = HEADER_3;
                else { i--; estadoProtocoloUdp = START; }
                break;
            case HEADER_3:
                if (ch == 'R') estadoProtocoloUdp = NBYTES;
                else { i--; estadoProtocoloUdp = START; }
                break;
            case NBYTES:
                rxDataUdp.nBytes = ch;
                estadoProtocoloUdp = TOKEN;
                break;
            case TOKEN:
                if (ch == ':') {
                    estadoProtocoloUdp = PAYLOAD;
                    rxDataUdp.cheksum  = 'U' ^ 'N' ^ 'E' ^ 'R' ^ rxDataUdp.nBytes ^ ':';
                    rxDataUdp.payLoad[0] = rxDataUdp.nBytes;
                    rxDataUdp.index = 1;
                } else {
                    i--; estadoProtocoloUdp = START;
                }
                break;
            case PAYLOAD:
                if (rxDataUdp.nBytes > 1) {
                    rxDataUdp.payLoad[rxDataUdp.index++] = ch;
                    rxDataUdp.cheksum ^= ch;
                }
                rxDataUdp.nBytes--;
                if (rxDataUdp.nBytes == 0) {
                    estadoProtocoloUdp = START;
                    // 'ch' es el checksum recibido en el último byte del paquete
                    if (rxDataUdp.cheksum == ch) {
                        decodeData(&rxDataUdp.payLoad[0], UDP);
                    } else {
                        ui->textEdit_RAW->append(" CHK DISTINTO!!!!! ");
                    }
                }
                break;
            default:
                estadoProtocoloUdp = START;
                break;
            } // switch
        } // for payload
    } // while
}

bool MainWindow::sendLineSpeedCommand(float speedMps)
{
    // Trama UNER: cmd (1) + float32 (4) + checksum (1) => NBYTES=6.
    _udat value;
    value.f32 = speedMps;
    QByteArray frame;
    frame.reserve(12);
    frame.append("UNER", 4);
    frame.append(char(0x06));
    frame.append(':');
    frame.append(char(MODIFY_LINE_SPEED));
    for (int i = 0; i < 4; ++i) frame.append(char(value.ui8[i]));

    quint8 checksum = 0;
    for (char byte : frame) checksum ^= static_cast<quint8>(byte);
    frame.append(char(checksum));

    qint64 sent = -1;
    QString channel;
    if (serial->isOpen() && serial->isWritable()) {
        sent = serial->write(frame);
        channel = "Serial";
    } else if (UdpSocket1->state() == QAbstractSocket::BoundState &&
               !clientAddress.isNull() && puertoremoto > 0) {
        sent = UdpSocket1->writeDatagram(frame, clientAddress,
                                         static_cast<quint16>(puertoremoto));
        channel = "UDP";
    } else {
        const QString msg = "VELOCIDAD LINEA: Sin conexion Serial/UDP disponible.";
        ui->textEdit_PROCCES->append(msg);
        if (lineSpeedStatusLabel) lineSpeedStatusLabel->setText("Sin conexion disponible");
        return false;
    }

    if (sent != frame.size()) {
        const QString msg = QString("VELOCIDAD LINEA: Error de envio por %1").arg(channel);
        ui->textEdit_PROCCES->append(msg);
        if (lineSpeedStatusLabel) lineSpeedStatusLabel->setText(msg);
        return false;
    }

    ui->textEdit_RAW->append(
        QString("PC--%1-->STM32 MODIFY_LINE_SPEED=%2 m/s [%3]")
            .arg(channel)
            .arg(speedMps, 0, 'f', 2)
            .arg(QString::fromLatin1(frame.toHex(' '))));
    return true;
}

void MainWindow::sendDataUDP()
{
    // --- Verificar que el socket esté realmente BIND (abierto para recibir) ---
    if (UdpSocket1->state() != QAbstractSocket::BoundState && UdpSocket1->localPort() == 0) {
        QMessageBox::warning(this, "UDP", "Primero abrí el puerto local (Open UDP).");
            return;
    }

    // --- Cargar/validar destino ---
    QHostAddress dest = clientAddress;
    if (dest.isNull()) {
        const QString ip = ui->lineEdit_IP_REMOTA->text().trimmed();
        if (!dest.setAddress(ip)) {
            QMessageBox::warning(this, "UDP", "IP remota inválida.");
                return;
        }
        clientAddress = dest; // recordar para próximos envíos
    }

    if (puertoremoto <= 0) {
        bool okp = false;
        puertoremoto = ui->lineEdit_DEVICEPORT->text().toUShort(&okp);
        if (!okp || puertoremoto == 0) {
            QMessageBox::warning(this, "UDP", "Falta/incorrecto el PORT del dispositivo.");
            return;
        }
    }

    // --- Armado de trama ---
    _udat w;
    uint8_t cmdId = static_cast<uint8_t>(ui->comboBox_CMD->currentData().toInt());
    unsigned char dato[256];
    int indice = 0;
    bool ok = false;

    // Encabezado "UNER" + NBYTES placeholder + ':'
    dato[indice++] = 'U';
    dato[indice++] = 'N';
    dato[indice++] = 'E';
    dato[indice++] = 'R';
    const int idxNbytes = indice;        // posición del campo NBYTES
    dato[indice++] = 0x00;               // se completa al final
    dato[indice++] = ':';                // token
    const int payloadStart = indice;     // inicio del payload

    // Payload según comando (misma lógica con switch)
    switch (cmdId) {
    case SETMOTORSPEED: {                // 0xA1
        dato[indice++] = SETMOTORSPEED;

        w.i32 = QInputDialog::getInt(this, "Velocidad", "Motor1:", 0, -100, 100, 1, &ok);
        if (!ok) return;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];

        w.i32 = QInputDialog::getInt(this, "Velocidad", "Motor2:", 0, -100, 100, 1, &ok);
        if (!ok) return;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case SETSERVOANGLE: {                // 0xA2
        dato[indice++] = SETSERVOANGLE;
        w.i32 = QInputDialog::getInt(this, "SERVO", "Angulo:", 0, -90, 90, 1, &ok);
        if (!ok) return;
        dato[indice++] = w.i8[0];
        break;
    }
    case MODIFYKP: { // MODIFYKP = 0xB1
        dato[indice++] = MODIFYKP;

        double kp_val = QInputDialog::getDouble(this, "Factor KP", "KP:", 0.0, 0.0, 1000.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kp_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFYKD: { // MODIFYKD = 0xB2
        dato[indice++] = MODIFYKD;

        double kd_val = QInputDialog::getDouble(this, "Factor KD", "KD:", 0.0, 0.0, 1000.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kd_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFYKI: { // MODIFYKI = 0xB3
        dato[indice++] = MODIFYKI;

        double ki_val = QInputDialog::getDouble(this, "Factor KI", "KI:", 0.0, 0.0, 1000.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)ki_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_BETA_G: { // MODIFY_BETA_G = 0xBC
        dato[indice++] = MODIFY_BETA_G;

        double beta_g_val = QInputDialog::getDouble(this, "Factor BETA G", "BETA_G:", 0.0, 0.0, 10.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)beta_g_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_BETA_A: { // MODIFY_BETA_A = 0xBD
        dato[indice++] = MODIFY_BETA_A;

        double beta_a_val = QInputDialog::getDouble(this, "Factor BETA A", "BETA_A:", 0.0, 0.0, 10.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)beta_a_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_KV_BRAKE: { // MODIFY_KV_BRAKE=0xBF
        dato[indice++] = MODIFY_KV_BRAKE;

        double kv_val = QInputDialog::getDouble(this, "Factor KV BRAKE", "KV_BRAKE:", 0.0, 0.0, 100.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kv_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_KP_LINE: { // MODIFY_KP_LINE=0xC0
        dato[indice++] = MODIFY_KP_LINE;

        double kp_val = QInputDialog::getDouble(this, "Factor KP seguidor de linea", "KP_LINE:", 0.0, 0.0, 100.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kp_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_KD_LINE: { // MODIFY_KD_LINE=0xC1
        dato[indice++] = MODIFY_KD_LINE;

        double kd_val = QInputDialog::getDouble(this, "Factor KD seguidor de linea", "KD_LINE:", 0.0, 0.0, 100.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kd_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_KI_LINE: { // MODIFY_KI_LINE=0xC2
        dato[indice++] = MODIFY_KI_LINE;

        double ki_val = QInputDialog::getDouble(this, "Factor KI seguidor de linea", "KI_LINE:", 0.0, 0.0, 10.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)ki_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_LINE_THRES: { // MODIFY_LINE_THRES=0xC3
        dato[indice++] = MODIFY_LINE_THRES;

        double ki_val = QInputDialog::getDouble(this, "Factor KI seguidor de linea", "KI_LINE:", 0.0, 0.0, 10.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)ki_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_LINE_SPEED: { // MODIFY_LINE_SPEED=0xC4
        dato[indice++] = MODIFY_LINE_SPEED;

        double angle_val = QInputDialog::getDouble(this, "Velocidad del seguidor de linea", "Objetivo (m/s):", 2.5, 0.2, 4.0, 2, &ok);
        if (!ok) return;
        requestedLineSpeedMps = static_cast<float>(angle_val);
        w.f32 = (float)angle_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_SETPOINT: {   // 0xC8
        dato[indice++] = MODIFY_SETPOINT;

        w.f32 = (float)ui->spinBox_SETPOINT->value();
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case GETALIVE:          // 0xF0
    case GETANGLE:          // 0xA7
    case GETADCVALUES:      // 0xA5
    case GETMPU6050VALUES:  // 0xA6
    case SENDALLSENSORS:    // 0xA9
    case GETDISTANCE:       // 0xA3
    case GETSPEED:          // 0xA4
    case GETSWITCHES:       // 0xA5
    case GETFIRMWARE:       // 0xF1
    case GETANALOGSENSORS:  // 0xA0
    case BALANCE:           //BALANCE=0xB4
    case RESETMASSCENTER:   // RESETMASSCENTER=0xB7
    case ACTIVATE_CSV_LOG:  // ACTIVATE_CSV_LOG=0xB9
    case ACTIVATE_WIFI_LOG: // ACTIVATE_WIFI_LOG=0xBA
    case CHANGE_DISPLAY:    // CHANGE_DISPLAY=0xBE
    case ACTIVATE_LINE_FOLLOWING:   // ACTIVATE_LINE_FOLLOWING = 0xC5
    case ACTIVATE_POS_MAINTENANCE:  //ACTIVATE_POS_MAINTENANCE = 0xC6
    case ACTIVATE_MANUAL_CONTROL:
    case MOVE_FORWARD:
    case MOVE_BACKWARD:
    case MOVE_LEFT:
    case MOVE_RIGHT:
    case MOVE_STOP:
    case SETLEDS:
    case GET_ODOMETRY:      // GET_ODOMETRY=0xDA
    case RESET_ODOMETRY:    // RESET_ODOMETRY=0xDB
        dato[indice++] = cmdId;
        break;

    default:
        QMessageBox::warning(this, "UDP", "Comando desconocido.");
        return;
    }

    // Longitud NBYTES = (cmd + datos) + 1 (checksum)
    const int payloadSinChk = indice - payloadStart;
    const unsigned char nbytes = static_cast<unsigned char>(payloadSinChk + 1);
    dato[idxNbytes] = nbytes;

    // Checksum: 'U'^'N'^'E'^'R'^NBYTES^':' ^ payload (sin el checksum)
    unsigned char chk = 'U' ^ 'N' ^ 'E' ^ 'R' ^ dato[idxNbytes] ^ ':';
    for (int i = payloadStart; i < indice; ++i) chk ^= dato[i];
    dato[indice++] = chk;

    // Envío: 4 + 1 + 1 + nbytes
    const int totalLen = 6 + nbytes;
    const qint64 sent = UdpSocket1->writeDatagram(
        reinterpret_cast<const char *>(dato),
        totalLen,
        clientAddress,
        static_cast<quint16>(puertoremoto)
        );

    // Log
    QString str;
    str.reserve(totalLen * 4);
    for (int i = 0; i < totalLen; ++i) {
        unsigned char b = dato[i];
        if (isalnum(b)) str += QChar(b);
        else str += "{" + QString("%1").arg(b, 2, 16, QChar('0')) + "}";
    }
    str += "  " + clientAddress.toString() + "  " + QString::number(puertoremoto);
    ui->textEdit_RAW->append("PC--UDP-->MBED ( " + str + " )");

    if (sent != totalLen) {
        ui->textEdit_RAW->append("** UDP TX error: " + UdpSocket1->errorString());
    }
}

void MainWindow::sendDataSerial(){
    uint8_t cmdId;
    _udat   w;
    bool ok;

    unsigned char dato[256];
    unsigned char indice=0, chk=0;

    QString str="";

    dato[indice++]='U';
    dato[indice++]='N';
    dato[indice++]='E';
    dato[indice++]='R';
    dato[indice++]=0x00;
    dato[indice++]=':';
    cmdId = ui->comboBox_CMD->currentData().toInt();
    switch (cmdId) {
    case SETMOTORSPEED://MOTORTEST=0xA1,
        dato[indice++] =SETMOTORSPEED;
        w.i32 = QInputDialog::getInt(this, "Velocidad", "Motor1:", 0, -100, 100, 1, &ok);
        if(!ok)
            break;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        w.i32 = QInputDialog::getInt(this, "Velocidad", "Motor2:", 0, -100, 100, 1, &ok);
        if(!ok)
            break;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        dato[NBYTES]= 0x0A;
        break;
    case SETSERVOANGLE://SERVOANGLE=0xA2,
        dato[indice++] =SETSERVOANGLE;
        w.i32 = QInputDialog::getInt(this, "SERVO", "Angulo:", 0, 0, 180, 1, &ok);
        if(!ok)
            break;
        dato[indice++] = w.i8[0];
        dato[NBYTES]= 0x03;
        break;
    case MODIFYKP: { // MODIFYKP = 0xB1
        dato[indice++] = MODIFYKP;

        double kp_val = QInputDialog::getDouble(this, "Factor KP", "KP:", 0.0, 0.0, 1000.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kp_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFYKD: { // MODIFYKD = 0xB2
        dato[indice++] = MODIFYKD;

        double kd_val = QInputDialog::getDouble(this, "Factor KD", "KD:", 0.0, 0.0, 1000.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kd_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFYKI: { // MODIFYKI = 0xB3
        dato[indice++] = MODIFYKI;

        double ki_val = QInputDialog::getDouble(this, "Factor KI", "KI:", 0.0, 0.0, 1000.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)ki_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_BETA_G: { // MODIFY_BETA_G = 0xBC
        dato[indice++] = MODIFY_BETA_G;

        double beta_g_val = QInputDialog::getDouble(this, "Factor BETA G", "BETA_G:", 0.0, 0.0, 10.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)beta_g_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_BETA_A: { // MODIFY_BETA_A = 0xBD
        dato[indice++] = MODIFY_BETA_A;

        double beta_a_val = QInputDialog::getDouble(this, "Factor BETA A", "BETA_A:", 0.0, 0.0, 10.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)beta_a_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_KV_BRAKE: { // MODIFY_KV_BRAKE=0xBF
        dato[indice++] = MODIFY_KV_BRAKE;

        double kv_val = QInputDialog::getDouble(this, "Factor KV BRAKE", "KV_BRAKE:", 0.0, 0.0, 100.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kv_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }  
    case MODIFY_KP_LINE: { // MODIFY_KP_LINE=0xC0
        dato[indice++] = MODIFY_KP_LINE;

        double kp_val = QInputDialog::getDouble(this, "Factor KP seguidor de linea", "KP_LINE:", 0.0, 0.0, 100.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kp_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_KD_LINE: { // MODIFY_KD_LINE=0xC1
        dato[indice++] = MODIFY_KD_LINE;

        double kd_val = QInputDialog::getDouble(this, "Factor KD seguidor de linea", "KD_LINE:", 0.0, 0.0, 10.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)kd_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_KI_LINE: { // MODIFY_KI_LINE=0xC2
        dato[indice++] = MODIFY_KI_LINE;

        double ki_val = QInputDialog::getDouble(this, "Factor KI seguidor de linea", "KI_LINE:", 0.0, 0.0, 10.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)ki_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_LINE_THRES: { // MODIFY_LINE_THRES=0xC3
        dato[indice++] = MODIFY_LINE_THRES;

        double ki_val = QInputDialog::getDouble(this, "Factor KI seguidor de linea", "KI_LINE:", 0.0, 0.0, 10.0, 3, &ok);
        if (!ok) return;
        w.f32 = (float)ki_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_LINE_SPEED: { // MODIFY_LINE_SPEED=0xC4
        dato[indice++] = MODIFY_LINE_SPEED;

        double angle_val = QInputDialog::getDouble(this, "Velocidad del seguidor de linea", "Objetivo (m/s):", 2.5, 0.2, 4.0, 2, &ok);
        if (!ok) return;
        requestedLineSpeedMps = static_cast<float>(angle_val);
        w.f32 = (float)angle_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case MODIFY_SETPOINT: {   // 0xC8
        dato[indice++] = MODIFY_SETPOINT;

        w.f32 = (float)ui->spinBox_SETPOINT->value();
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }
    case GETALIVE:
    case GETADCVALUES: //GETADCVALUES=0xA5
    case GETMPU6050VALUES:  //GETMPU6050VALUES=0xA6
    case GETANGLE: //GETANGLE = 0XA7
    case GETDISTANCE://GETDISTANCE=0xA3
    case GETSPEED://GETSPEED=0xA4
    case GETSWITCHES://GETSWITCHES=0xA5
    case GETFIRMWARE:// GETFIRMWARE=0xF1
    case GETANALOGSENSORS://ANALOGSENSORS=0xA0
    case SENDALLSENSORS: //SENDALLSENSORS=0xA9
    case STOPALLSENSORS: //STOPALLSENSORS=0xAA
    case BALANCE: //BALANCE=0xB4
    case RESETMASSCENTER:   // RESETMASSCENTER=0xB7
    case ACTIVATE_CSV_LOG:  // ACTIVATE_CSV_LOG=0xB9
    case ACTIVATE_WIFI_LOG: // ACTIVATE_WIFI_LOG=0xBA
    case CHANGE_DISPLAY :   // CHANGE_DISPLAY=0xBE
    case ACTIVATE_LINE_FOLLOWING: // ACTIVATE_LINE_FOLLOWING = 0xC5
    case ACTIVATE_POS_MAINTENANCE: //ACTIVATE_POS_MAINTENANCE = 0xC6
    case ACTIVATE_MANUAL_CONTROL:
    case MOVE_FORWARD:
    case MOVE_BACKWARD:
    case MOVE_LEFT:
    case MOVE_RIGHT:
    case MOVE_STOP:
    case SETLEDS:
    case GET_ODOMETRY:      // GET_ODOMETRY=0xDA
    case RESET_ODOMETRY:    // RESET_ODOMETRY=0xDB
        dato[indice++]=cmdId;
        //falta implementar el envío del valor de seteo
        dato[NBYTES]=0x02;
        break;
    default:
        return;
    }
    for(int a=0 ;a<indice;a++)
        chk^=dato[a];
    dato[indice]=chk;

    if(serial->isWritable()){
        serial->write(reinterpret_cast<char *>(dato),dato[NBYTES]+PAYLOAD);
    }

    for(int i=0; i<=indice; i++){
        if(isalnum(dato[i]))
            str = str + QString("%1").arg(char(dato[i]));
        else
            str = str +"{" + QString("%1").arg(dato[i],2,16,QChar('0')) + "}";
    }

    uint16_t valor=dato[NBYTES]+PAYLOAD;
    ui->textEdit_RAW->append("INDICE ** " +QString().number(indice,10) + " **" );
    ui->textEdit_RAW->append("NUMERO DE DATOS ** " +QString().number(valor,10) + " **" );
    ui->textEdit_RAW->append("CHECKSUM ** " +QString().number(chk,16) + " **" );
    ui->textEdit_RAW->append("PC--SERIAL-->MBED ( " + str + " )");

}

void MainWindow::decodeData(uint8_t *datosRx, uint8_t source){
    int32_t length = sizeof(*datosRx)/sizeof(datosRx[0]);
    QString str;
    _udat w;
    qreal t;

    uint16_t adcValues[8];
    int16_t ax, ay, az, gx, gy, gz;

    for(int i = 1; i<length; i++){
        if(isalnum(datosRx[i]))
            str = str + QString("%1").arg(char(datosRx[i]));
        else
            str = str +QString("%1").arg(datosRx[i],2,16,QChar('0'));
    }
    ui->textEdit_RAW->append("*(MBED-S->PC)->decodeData (" + str + ")");

    switch (datosRx[1]) {
    case GETALIVE://     GETALIVE=0xF0,
        if(datosRx[2]==ACK){
            if(source) {
                    contadorAlive++;
                    str="ALIVE BLUEPILL VIA *SERIE* RECIBIDO!!!";
                    ui->textEdit_PROCCES->append(str);
            } else {
                    contadorAlive++;
                    // El ping de latencia (sendAlivePing, cada 2s) usa este mismo comando —
                    // no se loguea acá para no llenar la consola con un mensaje cada 2s.
                    if (aliveSentTimer.isValid()) {
                        healthDashboard->onAlivePong(aliveSentTimer.elapsed());
                    }
            }
        } else {
            str= "ALIVE BLUEPILL VIA *SERIE*  NO ACK!!!";
            ui->textEdit_PROCCES->append(str);
        }
        break;
    case GETFIRMWARE://     GETFIRMWARE=0xF1
        str = "FIRMWARE:";
        for(uint8_t a=0;a<(datosRx[0]-1);a++){
            str += (QChar)datosRx[2+a];
        }
        ui->textEdit_PROCCES->append(str);

        break;
    case SETLEDS:
        str = "LD3: ";
        if(datosRx[2] & 0x08)
            str = str + "HIGH";
        else
            str = str + "LOW";
        str = str + " - LD2: ";
        if(datosRx[2] & 0x04)
            str = str + "HIGH";
        else
            str = str + "LOW";
        str = str + " - LD1: ";
        if(datosRx[2] & 0x02)
            str = str + "HIGH";
        else
            str = str + "LOW";
        str = str + " - LD0: ";
        if(datosRx[2] & 0x01)
            str = str + "HIGH";
        else
            str = str + "LOW";
        ui->textEdit_PROCCES->append(str);
        break;

    case GETADCVALUES: { //GETADCVALUES=0xA5
        w.ui8[0] = datosRx[2];  w.ui8[1] = datosRx[3];  adcValues[0] = w.ui16[0];
        w.ui8[0] = datosRx[4];  w.ui8[1] = datosRx[5];  adcValues[1] = w.ui16[0];
        w.ui8[0] = datosRx[6];  w.ui8[1] = datosRx[7];  adcValues[2] = w.ui16[0];
        w.ui8[0] = datosRx[8];  w.ui8[1] = datosRx[9];  adcValues[3] = w.ui16[0];
        w.ui8[0] = datosRx[10]; w.ui8[1] = datosRx[11]; adcValues[4] = w.ui16[0];
        w.ui8[0] = datosRx[12]; w.ui8[1] = datosRx[13]; adcValues[5] = w.ui16[0];
        w.ui8[0] = datosRx[14]; w.ui8[1] = datosRx[15]; adcValues[6] = w.ui16[0];
        w.ui8[0] = datosRx[16]; w.ui8[1] = datosRx[17]; adcValues[7] = w.ui16[0];

        str = "Valores de ADC obtenidos correctamente!";
        ui->textEdit_PROCCES->append(str);

        for (int i = 0; i < 8; ++i) {   // --- actualizo barras sin borrar/append, usando replace() ---
            uint16_t v = qMin<uint16_t>(adcValues[i], 4000);
            adcBarSet->replace(i, v);
        }

        adcChart->update();     // --- fuerzo repaint si hace falta ---
        break;
    }
    case GETMPU6050VALUES:  //GETMPU6050VALUES=0xA6,
        if (!mpuStarted) {
            mpuStarted = true;
            elapsedSec = 0.0;
            mpuPollTimer->start(SAMPLE_INTERVAL_MS);   // 20 Hz, ajusta a tu frecuencia de muestreo
        }

        w.ui8[0] = datosRx[2];  // AX
        w.ui8[1] = datosRx[3];
        lastAx= w.i16[0];

        w.ui8[0] = datosRx[4];  // AY
        w.ui8[1] = datosRx[5];
        lastAy = w.i16[0];

        w.ui8[0] = datosRx[6];  // AZ
        w.ui8[1] = datosRx[7];

        w.ui8[0] = datosRx[8];  // GX
        w.ui8[1] = datosRx[9];

        w.ui8[0] = datosRx[10];  // GY
        w.ui8[1] = datosRx[11];

        w.ui8[0] = datosRx[12];  // GZ
        w.ui8[1] = datosRx[13];

        str = "Valores de MPU6050 obtenidos correctamente!";
        ui->textEdit_PROCCES->append(str);

        elapsedSec += SAMPLE_INTERVAL_S;
        accAxisX->setRange(elapsedSec - 10, elapsedSec);

        updatePosition();
        break;
    case GETANGLE: {
        // ROLL
        w.ui8[0] = datosRx[2];
        w.ui8[1] = datosRx[3];
        w.ui8[2] = datosRx[4];
        w.ui8[3] = datosRx[5];
        float roll = w.f32;  // firmado
        QString strRoll = QString::number(roll, 'f', 2) + " \u00B0";
        ui->label_inclination_Xvalue->setText(strRoll);
        robotViewer3D->setPitch(roll);

        // PITCH
        w.ui8[0] = datosRx[6];
        w.ui8[1] = datosRx[7];
        w.ui8[2] = datosRx[8];
        w.ui8[3] = datosRx[9];
        float pitch = w.f32;
        QString strPitch = QString::number(pitch, 'f', 2) + " \u00B0";
        ui->label_inclination_Yvalue->setText(strPitch);

        ui->textEdit_PROCCES->append("Valores de inclinacion obtenidos correctamente!");
        break;
    }

    case GET_ODOMETRY: { // GET_ODOMETRY=0xDA — respuesta: x[m], y[m], theta[°]
        w.ui8[0] = datosRx[2];
        w.ui8[1] = datosRx[3];
        w.ui8[2] = datosRx[4];
        w.ui8[3] = datosRx[5];
        float odomX = w.f32;

        w.ui8[0] = datosRx[6];
        w.ui8[1] = datosRx[7];
        w.ui8[2] = datosRx[8];
        w.ui8[3] = datosRx[9];
        float odomY = w.f32;

        w.ui8[0] = datosRx[10];
        w.ui8[1] = datosRx[11];
        w.ui8[2] = datosRx[12];
        w.ui8[3] = datosRx[13];
        float odomTheta = w.f32;

        str = "ODOMETRIA -> X: " + QString::number(odomX, 'f', 3)
            + " m  Y: " + QString::number(odomY, 'f', 3)
            + " m  Theta: " + QString::number(odomTheta, 'f', 2) + " °";
        ui->textEdit_PROCCES->append(str);
        break;
    }
    case WIFI_ODOM_DATA: { // WIFI_ODOM_DATA=0xDC — push periódico (no pedido), no requiere ACTIVATE_WIFI_LOG
        // Retrocompatible (2026-07-11): lat_deg se agregó al FINAL del struct —
        // un firmware sin ese campo manda 4 bytes menos y sigue siendo válido
        // (lat=0). Solo se rechaza si ni siquiera trae el layout base.
        const quint8 kOdomBaseLen = sizeof(WifiOdomData_t) - sizeof(float); // sin lat_deg
        if (datosRx[0] < (kOdomBaseLen + 1)) {
            ui->textEdit_PROCCES->append("WIFI_ODOM_DATA: Packet too short!");
            break;
        }
        const bool odomHasLat = (datosRx[0] >= (sizeof(WifiOdomData_t) + 1));

        WifiOdomData_t *odata = reinterpret_cast<WifiOdomData_t*>(&datosRx[2]);
        healthDashboard->onOdomSeq(odata->seq);
        healthDashboard->onRobotState(odata->robot_state);
        updateOdomChart(odata->x_m, odata->y_m, odata->theta_deg, odata->line_detected != 0, odata->line_state,
                        odata->adc5, odata->adc6, odata->adc7, odata->adc8, odata->t_ms);
        // Balanceo + rumbo → Vista 3D: el modelo se inclina Y gira como el robot
        // real siempre que haya WiFi, sin necesidad de activar el log de control
        // (que sigue alimentando solo el pitch, a 10 Hz, y conserva este rumbo).
        // Si el modelo gira al revés que el robot, negar theta acá (misma
        // incógnita de signo que ODOM_THETA_SIGN, aún sin validar).
        // Tercer eje (2026-07-10): banking lateral. Si el modelo se inclina al
        // revés que el robot en este eje, negar lat_deg acá (incógnita de signo
        // hermana de ODOM_THETA_SIGN).
        robotViewer3D->setPose(odata->roll_deg, odata->theta_deg,
                               odomHasLat ? odata->lat_deg : 0.0f);
        // Obstáculo frente al modelo 3D: mismos sensores (ADC 5/6/8), mismo umbral
        // (3200) y mismo mapeo crudo ADC→m que la barrera del mapa de odometría.
        // La condición de ángulo (roll en [-90°, +30°]) la evalúa el visor sobre
        // la pose mostrada.
        {
            const uint16_t frontMin3D = qMin(odata->adc5, qMin(odata->adc6, odata->adc8));
            robotViewer3D->setFrontObstacle(frontMin3D < 3200,
                                            0.06f + 0.30f * (float)frontMin3D / 4095.0f);
        }
        ui->label_inclination_Xvalue->setText(QString::number(odata->roll_deg, 'f', 2) + " °");
        break;
    }
    case RESET_ODOMETRY: // RESET_ODOMETRY=0xDB — respuesta: ACK
        if (datosRx[2] == ACK)
            str = "ODOMETRIA RESETEADA (ACK)";
        else
            str = "RESET_ODOMETRY: NO ACK!";
        ui->textEdit_PROCCES->append(str);
        break;

    case MODIFYKP: { //    MODIFYKP=0xB1,
        w.ui8[0] = datosRx[2];  // KP VALUE - Byte 0
        w.ui8[1] = datosRx[3];  // KP VALUE - Byte 1
        w.ui8[2] = datosRx[4];  // KP VALUE - Byte 2
        w.ui8[3] = datosRx[5];  // KP VALUE - Byte 3
        float KP_Value = w.f32; // <--- CORREGIDO (sin [0])
        QString strKP = QString::number(KP_Value, 'f', 3); // Muestra con 3 decimales
        ui->label_KP->setText(strKP);

        w.ui8[0] = datosRx[6];  // KD VALUE - Byte 0
        w.ui8[1] = datosRx[7];  // KD VALUE - Byte 1
        w.ui8[2] = datosRx[8];  // KD VALUE - Byte 2
        w.ui8[3] = datosRx[9];  // KD VALUE - Byte 3
        float KD_Value = w.f32; // <--- CORREGIDO (sin [0])
        QString strKD = QString::number(KD_Value, 'f', 3); // Muestra con 3 decimales
        ui->label_KD->setText(strKD);

        w.ui8[0] = datosRx[10]; // KI VALUE - Byte 0
        w.ui8[1] = datosRx[11]; // KI VALUE - Byte 1
        w.ui8[2] = datosRx[12]; // KI VALUE - Byte 2
        w.ui8[3] = datosRx[13]; // KI VALUE - Byte 3
        float KI_Value = w.f32; // <--- CORREGIDO (sin [0])
        QString strKI = QString::number(KI_Value, 'f', 3); // Muestra con 3 decimales
        ui->label_KI->setText(strKI);
        ui->textEdit_PROCCES->append("Valores de PID obtenidos correctamente!");
        break;
    }
    case MODIFYKD: { //    MODIFYKD=0xB2,
        w.ui8[0] = datosRx[2];  // KP VALUE - Byte 0
        w.ui8[1] = datosRx[3];  // KP VALUE - Byte 1
        w.ui8[2] = datosRx[4];  // KP VALUE - Byte 2
        w.ui8[3] = datosRx[5];  // KP VALUE - Byte 3
        float KP_Value = w.f32; // <--- CORREGIDO
        QString strKP = QString::number(KP_Value, 'f', 3); // Muestra con 3 decimales
        ui->label_KP->setText(strKP);

        w.ui8[0] = datosRx[6];  // KD VALUE - Byte 0
        w.ui8[1] = datosRx[7];  // KD VALUE - Byte 1
        w.ui8[2] = datosRx[8];  // KD VALUE - Byte 2
        w.ui8[3] = datosRx[9];  // KD VALUE - Byte 3
        float KD_Value = w.f32; // <--- CORREGIDO
        QString strKD = QString::number(KD_Value, 'f', 3); // Muestra con 3 decimales
        ui->label_KD->setText(strKD);

        w.ui8[0] = datosRx[10]; // KI VALUE - Byte 0
        w.ui8[1] = datosRx[11]; // KI VALUE - Byte 1
        w.ui8[2] = datosRx[12]; // KI VALUE - Byte 2
        w.ui8[3] = datosRx[13]; // KI VALUE - Byte 3
        float KI_Value = w.f32; // <--- CORREGIDO
        QString strKI = QString::number(KI_Value, 'f', 3); // Muestra con 3 decimales
        ui->label_KI->setText(strKI);
        ui->textEdit_PROCCES->append("Valores de PID obtenidos correctamente!");
        break;
    }
    case MODIFYKI: { //    MODIFYKI=0xB3,
        w.ui8[0] = datosRx[2];  // KP VALUE - Byte 0
        w.ui8[1] = datosRx[3];  // KP VALUE - Byte 1
        w.ui8[2] = datosRx[4];  // KP VALUE - Byte 2
        w.ui8[3] = datosRx[5];  // KP VALUE - Byte 3
        float KP_Value = w.f32; // <--- CORREGIDO
        QString strKP = QString::number(KP_Value, 'f', 3); // Muestra con 3 decimales
        ui->label_KP->setText(strKP);

        w.ui8[0] = datosRx[6];  // KD VALUE - Byte 0
        w.ui8[1] = datosRx[7];  // KD VALUE - Byte 1
        w.ui8[2] = datosRx[8];  // KD VALUE - Byte 2
        w.ui8[3] = datosRx[9];  // KD VALUE - Byte 3
        float KD_Value = w.f32; // <--- CORREGIDO
        QString strKD = QString::number(KD_Value, 'f', 3); // Muestra con 3 decimales
        ui->label_KD->setText(strKD);

        w.ui8[0] = datosRx[10]; // KI VALUE - Byte 0
        w.ui8[1] = datosRx[11]; // KI VALUE - Byte 1
        w.ui8[2] = datosRx[12]; // KI VALUE - Byte 2
        w.ui8[3] = datosRx[13]; // KI VALUE - Byte 3
        float KI_Value = w.f32; // <--- CORREGIDO
        QString strKI = QString::number(KI_Value, 'f', 3); // Muestra con 3 decimales
        ui->label_KI->setText(strKI);
        ui->textEdit_PROCCES->append("Valores de PID obtenidos correctamente!");
        break;
    }
    case SETMOTORSPEED://     SETMOTORSPEED=0xA1,
        if(datosRx[2]==0x0D)
            str= "Test Motores ACK";
        ui->textEdit_PROCCES->append(str);
        break;
    case SENDALLSENSORS: { //SENDALLSENSORS=0xA9
        if (!mpuStarted) {
            mpuStarted = true;
            elapsedSec = 0.0;
            mpuPollTimer->start(SAMPLE_INTERVAL_MS);   // 20 Hz, ajusta a tu frecuencia de muestreo
        }

        t = elapsedSec;

        w.ui8[0] = datosRx[2];  w.ui8[1] = datosRx[3];  adcValues[0] = w.ui16[0];
        w.ui8[0] = datosRx[4];  w.ui8[1] = datosRx[5];  adcValues[1] = w.ui16[0];
        w.ui8[0] = datosRx[6];  w.ui8[1] = datosRx[7];  adcValues[2] = w.ui16[0];
        w.ui8[0] = datosRx[8];  w.ui8[1] = datosRx[9];  adcValues[3] = w.ui16[0];
        w.ui8[0] = datosRx[10]; w.ui8[1] = datosRx[11]; adcValues[4] = w.ui16[0];
        w.ui8[0] = datosRx[12]; w.ui8[1] = datosRx[13]; adcValues[5] = w.ui16[0];
        w.ui8[0] = datosRx[14]; w.ui8[1] = datosRx[15]; adcValues[6] = w.ui16[0];
        w.ui8[0] = datosRx[16]; w.ui8[1] = datosRx[17]; adcValues[7] = w.ui16[0];

        w.ui8[0] = datosRx[18];  // AX
        w.ui8[1] = datosRx[19];
        ax = w.i16[0];
        lastAx = w.i16[0];

        w.ui8[0] = datosRx[20];  // AY
        w.ui8[1] = datosRx[21];
        ay = w.i16[0];
        lastAy = w.i16[0];

        w.ui8[0] = datosRx[22];  // AZ
        w.ui8[1] = datosRx[23];
        az = w.i16[0];

        w.ui8[0] = datosRx[24];  // GX
        w.ui8[1] = datosRx[25];
        gx = w.i16[0];

        w.ui8[0] = datosRx[26];  // GY
        w.ui8[1] = datosRx[27];
        gy = w.i16[0];

        w.ui8[0] = datosRx[28];  // GZ
        w.ui8[1] = datosRx[29];
        gz = w.i16[0];

        // ROLL
        w.ui8[0] = datosRx[30];
        w.ui8[1] = datosRx[31];
        qint16 roll = w.i16[0];  // firmado
        QString strRoll = QString::number(roll) + " \u00B0";
        ui->label_inclination_Xvalue->setText(strRoll);
        robotViewer3D->setPitch(static_cast<float>(roll));

        // PITCH
        w.ui8[0] = datosRx[32];
        w.ui8[1] = datosRx[33];
        qint16 pitch = w.i16[0];
        QString strPitch = QString::number(pitch) + " \u00B0";
        ui->label_inclination_Yvalue->setText(strPitch);


        for (int i = 0; i < 8; ++i) {   // --- actualizo barras sin borrar/append, usando replace() ---
            uint16_t v = qMin<uint16_t>(adcValues[i], 4000);
            adcBarSet->replace(i, v);
        }


        adcChart->update();     // --- fuerzo repaint si hace falta ---

        seriesAx->append(t, ax);
        seriesAy->append(t, ay);
        seriesAz->append(t, az);
        seriesGx->append(t, gx);
        seriesGy->append(t, gy);
        seriesGz->append(t, gz);

        // Avanzamos el tiempo:
        elapsedSec += SAMPLE_INTERVAL_S;       // si tus muestras vienen cada 30 ms
        accAxisX->setRange(elapsedSec - 10, elapsedSec);

        updatePosition();

        ui->textEdit_PROCCES->append("Valores periodicos obtenidos!");
        break;
    }
    case BALANCE:
        if(datosRx[2]==ACK){
            str="Se ha cambiado la bandera de balance correctamente!";
            isBalanceActive = !isBalanceActive;
            setToggleState(ui->pushButton_BALANCE, isBalanceActive ? "on" : "off");
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case RESETMASSCENTER:
        if(datosRx[2]==ACK){
            str="Se ha reestablecido el punto de balance correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case ACTIVATE_CSV_LOG:
        if(datosRx[2]==ACK){
            str="Se ha cambiado el estado de la bandera de CSV_LOG correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case ACTIVATE_WIFI_LOG:
        if(datosRx[2]==ACK){
            str="Se ha cambiado el estado de la bandera de WIFI_LOG correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case WIFI_LOG_DATA: {
        if (datosRx[0] < (sizeof(WifiLogData_t) + 1)) {
            ui->textEdit_PROCCES->append("WIFI_LOG_DATA: Packet too short!");
            break;
        }

        WifiLogData_t *data = reinterpret_cast<WifiLogData_t*>(&datosRx[2]);

        // 1. UI Labels
        ui->label_inclination_Xvalue->setText(
        QString::number(data->roll_filt, 'f', 2) + " °");
        robotViewer3D->setPitch(data->roll_filt);

        // 2. Tiempo
        double t_sec = data->t_ms / 1000.0;
        double minX  = t_sec - 10.0;
        double maxX  = t_sec;

        // 3. Gráficos
        seriesRollFilt->append(t_sec, data->roll_filt);
        seriesDynSp->append(t_sec, data->dyn_sp);
        seriesP->append(t_sec, data->p_term);
        seriesI->append(t_sec, data->i_term);
        seriesD->append(t_sec, data->d_term);
        seriesOutput->append(t_sec, data->output);
        seriesMR->append(t_sec, data->mR);
        seriesML->append(t_sec, data->mL);
        seriesDtCtrl->append(t_sec, data->dt_ctrl_us);

        // --- Gráficos Seguidor de Línea ---
        seriesLineError->append(t_sec, data->line_error);
        seriesLineP->append(t_sec, data->p_line);
        seriesLineI->append(t_sec, data->i_line);
        seriesLineD->append(t_sec, data->d_line);
        seriesLineSteering->append(t_sec, data->steering_adjustment);

        seriesLineAdc1->append(t_sec, data->adc1);
        seriesLineAdc2->append(t_sec, data->adc2);
        seriesLineAdc3->append(t_sec, data->adc3);
        seriesLineAdc4->append(t_sec, data->adc4);

        // 4. Ejes X
        accAxisX->setRange(minX, maxX);
        pidAxisX->setRange(minX, maxX);
        motorsAxisX->setRange(minX, maxX);
        systemAxisX->setRange(minX, maxX);  // ← NUEVO
        lineFollowerPidAxisX->setRange(minX, maxX);
        lineFollowerAdcAxisX->setRange(minX, maxX);

        // 5. Auto-escala IMU
        qreal minAcc = 1000.0, maxAcc = -1000.0;
        for (const QPointF &p : seriesRollFilt->points()) {
            if (p.x() >= minX) {
                    minAcc = qMin(minAcc, p.y());
                    maxAcc = qMax(maxAcc, p.y());
            }
        }
        if (maxAcc > minAcc) {
            qreal margin = (maxAcc - minAcc) * 0.1;
            if (margin == 0) margin = 1.0;
            accAxisY->setRange(minAcc - margin, maxAcc + margin);
        }

        // 6. Auto-escala PID
        qreal minPid = 1000000.0, maxPid = -1000000.0;
        for (auto *s : {seriesP, seriesI, seriesD, seriesOutput}) {
            for (const QPointF &p : s->points()) {
                    if (p.x() >= minX) {
                    minPid = qMin(minPid, p.y());
                    maxPid = qMax(maxPid, p.y());
                    }
            }
        }
        if (minPid != 1000000.0 && maxPid != -1000000.0) {
            qreal margin = (maxPid - minPid) * 0.1;
            if (margin == 0) margin = 0.5;
            pidAxisY->setRange(minPid - margin, maxPid + margin);
        }

        // --- Auto-escala Line Follower PID ---
        qreal minLinePid = 1000000.0, maxLinePid = -1000000.0;
        for (auto *s : {seriesLineError, seriesLineP, seriesLineI, seriesLineD, seriesLineSteering}) {
            for (const QPointF &p : s->points()) {
                if (p.x() >= minX) {
                    minLinePid = qMin(minLinePid, p.y());
                    maxLinePid = qMax(maxLinePid, p.y());
                }
            }
        }
        if (minLinePid != 1000000.0 && maxLinePid != -1000000.0) {
            qreal margin = (maxLinePid - minLinePid) * 0.1;
            if (margin == 0) margin = 0.5;
            lineFollowerPidAxisY->setRange(minLinePid - margin, maxLinePid + margin);
        }

        // 7. Motores Y fijo
        motorsAxisY->setRange(-110, 110);

        // 8. Sistema Y fijo
        systemAxisY->setRange(0, 35000);  // ← NUEVO

        // 9. Grabación CSV
        if (isRecording && csvLogFile.isOpen()) {
            QTextStream stream(&csvLogFile);
            stream << data->t_ms    << ","   // t_ms        ✅
               << 0                 << ","   // dt_us       ❌
               << data->dt_ctrl_us  << ","   // dt_ctrl_us  ✅
               << 0                 << ","   // accel_roll  ❌
               << 0                 << ","   // accel_roll_f❌
               << 0                 << ","   // gyro_y      ❌
               << 0                 << ","   // gyro_f      ❌
               << data->roll_filt   << ","   // roll_filt   ✅
               << data->dyn_sp      << ","   // dyn_sp      ✅
               << 0                 << ","   // error       ❌
               << data->p_term      << ","   // p           ✅
               << data->i_term      << ","   // i           ✅
               << data->d_term      << ","   // d           ✅
               << data->output      << ","   // output      ✅
               << 0                 << ","   // pwm_cmd     ❌
               << 0                 << ","   // pwm_sat     ❌
               << 0                 << ","   // sat_flag    ❌
               << data->mR          << ","   // mR          ✅
               << data->mL          << ","   // mL          ✅
               << 0                 << ","   // pitch       ❌
               << 0                 << ","   // ax          ❌
               << 0                 << ","   // ay          ❌
               << 0                 << ","   // az          ❌
               << 0                 << ","   // gx          ❌
               << 0                 << ","   // gy          ❌
               << 0                 << ","   // gz          ❌
               << data->line_error  << ","   // line_error  ✅
               << data->p_line      << ","   // p_line      ✅
               << data->i_line      << ","   // i_line      ✅
               << data->d_line      << ","   // d_line      ✅
               << data->steering_adjustment << "," // steering_adjustment ✅
               << data->adc1        << ","   // adc1        ✅
               << data->adc2        << ","   // adc2        ✅
               << data->adc3        << ","   // adc3        ✅
               << data->adc4        << "\n"; // adc4        ✅
        }
        break;
    }
    case MODIFY_BETA_G:
        if(datosRx[2]==ACK){
            str="Se ha modificado el valor de BETA_G correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case MODIFY_BETA_A:
        if(datosRx[2]==ACK){
            str="Se ha modificado el valor de BETA_A correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case CHANGE_DISPLAY :   // CHANGE_DISPLAY=0xBE
        if(datosRx[2]==ACK){
            str="Se ha intercambiado el display correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case MODIFY_KV_BRAKE :   // MODIFY_KV_BRAKE=0xBF
        if(datosRx[2]==ACK){
            str="Se ha modificado el display correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case MODIFY_KP_LINE :   // MODIFY_KP_LINE=0xC0
        if(datosRx[2]==ACK){
            str="Se ha modificado el valor de KP_LINE correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case MODIFY_KD_LINE :   // MODIFY_KD_LINE=0xC1
        if(datosRx[2]==ACK){
            str="Se ha modificado el valor de KD_LINE correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case MODIFY_KI_LINE :   // MODIFY_KI_LINE=0xC2
        if(datosRx[2]==ACK){
            str="Se ha modificado el valor de KI_LINE correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case MODIFY_LINE_THRES :   // MODIFY_LINE_THRES=0xC3
        if(datosRx[2]==ACK){
            str="Se ha modificado el valor de LINE_THRES correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case MODIFY_LINE_SPEED :   // MODIFY_LINE_SPEED=0xC4
        if(datosRx[2]==ACK){
            str=QString("Velocidad de linea aplicada: %1 m/s").arg(requestedLineSpeedMps, 0, 'f', 2);
            if (lineSpeedStatusLabel) lineSpeedStatusLabel->setText(str);
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case ACTIVATE_LINE_FOLLOWING :   // ACTIVATE_LINE_FOLLOWING=0xC5
        if(datosRx[2]==ACK){
            str="Se ha modificado el valor de la bandera de seguidor de LINEA correctamente!";
            isFollowLineActive = !isFollowLineActive;
            setToggleState(ui->pushButton_FOLLOW_LINE, isFollowLineActive ? "on" : "off");
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case ACTIVATE_POS_MAINTENANCE :   // ACTIVATE_POS_MAINTENANCE=0xC6
        if(datosRx[2]==ACK){
            str="Se ha modificado el valor de la bandera de mantencion de posicion correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case ACTIVATE_MANUAL_CONTROL :
        if(datosRx[2]==ACK){
            str="Se ha modificado el estado de control manual correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case MODIFY_SETPOINT :
        if(datosRx[2]==ACK){
            str="Se ha modificado el SEPOINT correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case MOVE_FORWARD:
    case MOVE_BACKWARD:
    case MOVE_LEFT:
    case MOVE_RIGHT:
    case MOVE_STOP:
        if(datosRx[2]==ACK){
            // No logguear cada comando de movimiento para no llenar la consola si es a 20Hz
            // str="Comando de movimiento manual aceptado.";
        }
        break;
    default:
        str = str + "Comando DESCONOCIDO!!!!";
        ui->textEdit_PROCCES->append(str);
    }
}

void MainWindow::on_pushButton_clicked()    // BOTON DE "LIMPIAR PANTALLAS"
{
    resetConsoleHeaders(ui->textEdit_RAW, ui->textEdit_PROCCES);
}

void MainWindow::updatePlot() {
    // 1) Desplaza la ventana de tiempo en ambos ejes X
    accAxisX->setRange(elapsedSec - 10.0, elapsedSec);
    gyroAxisX->setRange(elapsedSec - 10.0, elapsedSec);
    rawSensorsAccAxisX->setRange(elapsedSec - 10.0, elapsedSec);
    rawSensorsGyroAxisX->setRange(elapsedSec - 10.0, elapsedSec);

    // 2) Calcula mínimo y máximo sólo para las series de aceleración
    qreal minAcc = std::numeric_limits<qreal>::max();
    qreal maxAcc = std::numeric_limits<qreal>::lowest();
    for (auto *s : {seriesAx, seriesAy, seriesAz, seriesRollFilt, seriesPitch, seriesAccelRollF}) {
        for (const QPointF &p : s->points()) {
            minAcc = qMin(minAcc, p.y());
            maxAcc = qMax(maxAcc, p.y());
        }
    }
    // 3) Aplica un 10 % de margen y actualiza el eje Y del acelerómetro
    if (maxAcc > minAcc) {
        qreal mAcc = (maxAcc - minAcc) * 0.1;
        accAxisY->setRange(minAcc - mAcc, maxAcc + mAcc);
    }

    // 4) Igual para las series de giroscopio
    qreal minGyr = std::numeric_limits<qreal>::max();
    qreal maxGyr = std::numeric_limits<qreal>::lowest();
    for (auto *s : {seriesGx, seriesGy, seriesGz, seriesGyroF}) {
        for (const QPointF &p : s->points()) {
            minGyr = qMin(minGyr, p.y());
            maxGyr = qMax(maxGyr, p.y());
        }
    }
    if (maxGyr > minGyr) {
        qreal mGyr = (maxGyr - minGyr) * 0.1;
        gyroAxisY->setRange(minGyr - mGyr, maxGyr + mGyr);
    }
}

void MainWindow::updatePosition()
{
    double px = lastAx * posScale;
    double py = lastAy * posScale;

    if (positionSeries->count() == 0) {
        positionSeries->append(px, py);
    } else {
        positionSeries->replace(0, QPointF(px, py));
    }
}


void MainWindow::updateOdomChart(float x, float y, float thetaDeg, bool lineDetected, uint8_t lineState,
                                 uint16_t adc5, uint16_t adc6, uint16_t adc7, uint16_t adc8, uint32_t t_ms)
{
    // Odómetro + velocidad estimada entre paquetes (con piso de 3 mm por muestra
    // para no acumular el ruido de cuantización de la odometría con el robot quieto).
    if (odomHasPrev) {
        const double step = std::hypot((double)x - odomLastX, (double)y - odomLastY);
        if (step > 0.003)
            odomTotalDist += step;
        const quint32 dt_ms = t_ms - odomPrevTms;
        odomSpeed = (dt_ms > 0 && dt_ms < 5000) ? (step / (dt_ms / 1000.0)) : 0.0;
    }
    odomPrevTms = t_ms;

    odomTrailSeries->append(x, y);

    // ─── Barrera/obstáculo frente al robot, desde los sensores de objeto ───
    // Convención de los sensores: menos ADC = más cerca, ~4095 = nada.
    // ADC5/6/8 miran adelante; ADC7 es el lateral (izquierdo tras el giro de esquive).
    // La distancia real no está calibrada: se usa un mapeo lineal crudo ADC→metros
    // (0.06m pegado, ~0.35m en el umbral de detección) — suficiente para VISUALIZAR
    // que está viendo algo y de qué lado, no para medir.
    {
        const uint16_t OBJ_SEEN_ADC  = 3200;   // mismo umbral de detección del firmware
        const uint16_t WALL_SEEN_ADC = 3600;   // mismo umbral de pared visible (ADC7)
        const float th = qDegreesToRadians(thetaDeg);
        const float fx = qCos(th),  fy = qSin(th);    // adelante
        const float lx = -qSin(th), ly = qCos(th);    // izquierda (si el lateral aparece del lado equivocado, invertir este signo junto con ODOM_THETA_SIGN)
        auto adcToDist = [](uint16_t a) { return 0.06f + 0.30f * (float)a / 4095.0f; };

        uint16_t frontMin = qMin(adc5, qMin(adc6, adc8));
        odomBarrierSeries->clear();
        if (frontMin < OBJ_SEEN_ADC) {
            float d  = adcToDist(frontMin);
            float bx = x + d * fx, by = y + d * fy;
            odomBarrierSeries->append(bx - 0.10f * lx, by - 0.10f * ly);
            odomBarrierSeries->append(bx + 0.10f * lx, by + 0.10f * ly);
            odomObstaclePts->append(bx, by);
        }
        if (adc7 < WALL_SEEN_ADC) {
            float d7 = adcToDist(adc7);
            odomObstaclePts->append(x + d7 * lx, y + d7 * ly);
        }
        // Cap para que el rastro no crezca sin límite en sesiones largas
        if (odomObstaclePts->count() > 3000)
            odomObstaclePts->removePoints(0, odomObstaclePts->count() - 3000);
    }

    // "Cinta": mientras hay línea detectada se va agregando al segmento en curso
    // (grueso y negro); al perderla se cierra el segmento (nullptr) para que la
    // próxima vez que se detecte línea arranque un segmento nuevo — así no queda
    // una línea recta uniendo dos tramos de cinta separados por un tramo sin línea.
    if (lineDetected) {
        if (!odomTapeCurrent) {
            odomTapeCurrent = new QLineSeries();
            QPen tapePen(QColor("#e6edf3")); // clara: la "cinta" debe verse sobre el mapa oscuro
            tapePen.setWidth(6);
            tapePen.setCapStyle(Qt::RoundCap);
            odomTapeCurrent->setPen(tapePen);
            if (odomTapeSegments.isEmpty())
                odomTapeCurrent->setName("Cinta (línea)");

            odomChart->addSeries(odomTapeCurrent);
            odomTapeCurrent->attachAxis(odomAxisX);
            odomTapeCurrent->attachAxis(odomAxisY);

            if (!odomTapeSegments.isEmpty()) {
                // Ocultar de la leyenda todos los segmentos salvo el primero, para
                // no llenarla con una entrada "Cinta (línea)" por cada tramo.
                const auto markers = odomChart->legend()->markers(odomTapeCurrent);
                if (!markers.isEmpty()) markers.first()->setVisible(false);
            }
            odomTapeSegments.append(odomTapeCurrent);
        }
        odomTapeCurrent->append(x, y);
    } else {
        odomTapeCurrent = nullptr;
    }

    if (odomCurrentSeries->count() == 0)
        odomCurrentSeries->append(x, y);
    else
        odomCurrentSeries->replace(0, QPointF(x, y));

    // Eventos: se anotan solo en la transición, no en cada muestra, para no
    // llenar el mapa de marcadores repetidos mientras el robot sigue en el mismo
    // estado (p.ej. todo el tiempo que dura la secuencia de esquive de un objeto).
    if (odomHasPrev) {
        if (odomPrevLineDetected && !lineDetected) {
            addOdomAnnotation(x, y, "Línea perdida", OdomEventLineLost);
        }
        if (odomPrevLineState < ODOM_LINE_STATE_OBJ_FIRST && lineState >= ODOM_LINE_STATE_OBJ_FIRST) {
            addOdomAnnotation(x, y, "Objeto", OdomEventObject);
        }
    }
    odomPrevLineDetected = lineDetected;
    odomPrevLineState    = lineState;
    odomHasPrev          = true;

    odomLastX     = x;
    odomLastY     = y;
    odomLastTheta = thetaDeg;

    if (odomFollowRobot) {
        // "Seguir robot": la vista se recentra en la posición actual conservando
        // el ancho de ventana que haya elegido el usuario con el zoom.
        const double spanX = odomAxisX->max() - odomAxisX->min();
        const double spanY = odomAxisY->max() - odomAxisY->min();
        odomAxisX->setRange(x - spanX / 2.0, x + spanX / 2.0);
        odomAxisY->setRange(y - spanY / 2.0, y + spanY / 2.0);
    } else {
        // Autoescala con margen, igual que el mapa 2D del display OLED (P7): el rango
        // solo crece, nunca se achica, para no reescalar el gráfico constantemente.
        bool rescale = false;
        if (x - 0.3 < odomMinX) { odomMinX = x - 0.3; rescale = true; }
        if (x + 0.3 > odomMaxX) { odomMaxX = x + 0.3; rescale = true; }
        if (y - 0.3 < odomMinY) { odomMinY = y - 0.3; rescale = true; }
        if (y + 0.3 > odomMaxY) { odomMaxY = y + 0.3; rescale = true; }
        if (rescale) {
            odomAxisX->setRange(odomMinX, odomMaxX);
            odomAxisY->setRange(odomMinY, odomMaxY);
        }
    }

    lblOdomInfo->setText(QString::fromUtf8("X: %1 m   Y: %2 m   θ: %3°   Línea: %4   |   Recorrido: %5 m   Vel: %6 m/s")
                              .arg(x, 0, 'f', 2)
                              .arg(y, 0, 'f', 2)
                              .arg(thetaDeg, 0, 'f', 1)
                              .arg(lineDetected ? "SI" : "NO")
                              .arg(odomTotalDist, 0, 'f', 2)
                              .arg(odomSpeed, 0, 'f', 2));

    // Cualquier cambio de rango de ejes (autoescala de arriba) mueve en pantalla
    // los puntos de las anotaciones; recalcular sus posiciones siempre al final.
    repositionOdomAnnotations();
}

void MainWindow::addOdomAnnotation(double x, double y, const QString &text, OdomEventType type)
{
    const QColor color = (type == OdomEventLineLost) ? QColor(Qt::red) : QColor(255, 140, 0);

    auto *item = new QGraphicsSimpleTextItem(text);
    item->setBrush(QBrush(color));
    QFont f = item->font();
    f.setBold(true);
    f.setPointSize(8);
    item->setFont(f);
    odomChart->scene()->addItem(item);

    OdomAnnotation a{x, y, item};
    odomAnnotations.append(a);

    if (type == OdomEventLineLost)
        odomLostMarkers->append(x, y);
    else
        odomObjMarkers->append(x, y);

    repositionOdomAnnotations();
}

void MainWindow::repositionOdomAnnotations()
{
    for (const OdomAnnotation &a : qAsConst(odomAnnotations)) {
        const QPointF scenePos = odomChart->mapToPosition(QPointF(a.x, a.y), odomTrailSeries);
        // Chico offset para que el texto no tape el marcador de color.
        a.item->setPos(scenePos.x() + 8, scenePos.y() - 16);
    }

    if (odomHasPrev)
        updateHeadingArrow(odomLastX, odomLastY, odomLastTheta);

    updateOdomScaleBar();

    // Texto de la medición: sigue al punto medio de la línea amarilla.
    if (odomMeasureText && odomMeasureSeries->count() == 2) {
        const QPointF a = odomMeasureSeries->at(0);
        const QPointF b = odomMeasureSeries->at(1);
        const QPointF midScene = odomChart->mapToPosition(
            QPointF((a.x() + b.x()) / 2.0, (a.y() + b.y()) / 2.0), odomTrailSeries);
        odomMeasureText->setPos(midScene.x() + 8, midScene.y() - 18);
    }
}

// Barra de escala abajo a la izquierda del plot area: elige un largo "redondo"
// en metros (0.1/0.2/0.5/1/2/5...) que ocupe entre ~60 y ~150 px en pantalla.
void MainWindow::updateOdomScaleBar()
{
    if (!odomScaleLine || !odomScaleText)
        return;

    const QRectF plot = odomChart->plotArea();
    if (plot.width() < 50) return;

    // px por metro con la escala actual del eje X
    const QPointF p0 = odomChart->mapToPosition(QPointF(0.0, 0.0), odomTrailSeries);
    const QPointF p1 = odomChart->mapToPosition(QPointF(1.0, 0.0), odomTrailSeries);
    const double pxPerM = std::abs(p1.x() - p0.x());
    if (pxPerM <= 0.0) return;

    static const double niceSteps[] = {0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0};
    double best = niceSteps[0];
    for (double s : niceSteps) {
        if (s * pxPerM <= 150.0) best = s;   // el más grande que entre en 150 px
    }
    const double lenPx = best * pxPerM;

    const double xLeft  = plot.left() + 12;
    const double yBase  = plot.bottom() - 14;
    odomScaleLine->setLine(QLineF(xLeft, yBase, xLeft + lenPx, yBase));
    odomScaleText->setText(best < 1.0 ? QString("%1 cm").arg(best * 100.0, 0, 'f', 0)
                                      : QString("%1 m").arg(best, 0, 'f', 0));
    odomScaleText->setPos(xLeft, yBase - 16);
}

// viewport de la vista → valores de ejes (metros) del mapa.
QPointF MainWindow::odomViewPosToValue(const QPointF &viewPos) const
{
    const QPointF scenePos = odomChartView->mapToScene(viewPos.toPoint());
    const QPointF chartPos = odomChart->mapFromScene(scenePos);
    return odomChart->mapToValue(chartPos, odomTrailSeries);
}

void MainWindow::onOdomHover(const QPointF &viewPos)
{
    if (!lblOdomCursor)
        return;
    const QPointF scenePos = odomChartView->mapToScene(viewPos.toPoint());
    if (!odomChart->plotArea().contains(odomChart->mapFromScene(scenePos))) {
        lblOdomCursor->setText("( --.-- , --.-- ) m");
        return;
    }
    const QPointF v = odomViewPosToValue(viewPos);
    lblOdomCursor->setText(QString("( %1 , %2 ) m")
                               .arg(v.x(), 6, 'f', 2)
                               .arg(v.y(), 6, 'f', 2));

    // Vista previa de la medición: con el primer punto fijado, la línea sigue
    // al cursor hasta el segundo click (igual que la herramienta de un CAD).
    if (odomChartView->measureMode() && odomMeasureHasP1 && !odomMeasureDone) {
        odomMeasureSeries->clear();
        odomMeasureSeries->append(odomMeasureP1);
        odomMeasureSeries->append(v);
        const double dist = std::hypot(v.x() - odomMeasureP1.x(), v.y() - odomMeasureP1.y());
        odomMeasureText->setText(QString("%1 m").arg(dist, 0, 'f', 3));
        repositionOdomAnnotations();
    }
}

void MainWindow::onOdomMeasureClick(const QPointF &viewPos)
{
    const QPointF v = odomViewPosToValue(viewPos);

    if (!odomMeasureHasP1 || odomMeasureDone) {
        // Primer punto (o arranque de una medición nueva sobre una cerrada)
        odomMeasureP1     = v;
        odomMeasureHasP1  = true;
        odomMeasureDone   = false;
        odomMeasureSeries->clear();
        odomMeasureSeries->append(v);
        odomMeasureSeries->append(v);
        odomMeasureText->setText("");
        return;
    }

    // Segundo punto: cerrar la medición
    odomMeasureSeries->clear();
    odomMeasureSeries->append(odomMeasureP1);
    odomMeasureSeries->append(v);
    const double dist = std::hypot(v.x() - odomMeasureP1.x(), v.y() - odomMeasureP1.y());
    odomMeasureText->setText(QString("%1 m").arg(dist, 0, 'f', 3));
    odomMeasureDone = true;
    repositionOdomAnnotations();
}

void MainWindow::saveOdomMapPng()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Guardar mapa como PNG",
        QString("mapa_odometria_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "Imagen PNG (*.png)");
    if (path.isEmpty())
        return;
    odomChartView->grab().save(path, "PNG");
    ui->textEdit_PROCCES->append("Mapa guardado en: " + path);
}

void MainWindow::updateHeadingArrow(double x, double y, double thetaDeg)
{
    // Largo de la flecha proporcional a lo que se ve del mapa en este momento (no
    // fijo en metros), para que sea igual de visible zoomeado o alejado.
    const double rangeX = odomAxisX->max() - odomAxisX->min();
    const double rangeY = odomAxisY->max() - odomAxisY->min();
    const double len = 0.08 * std::max(rangeX, rangeY);

    const double thetaRad = qDegreesToRadians(thetaDeg);
    const QPointF tip(x + len * std::cos(thetaRad), y + len * std::sin(thetaRad));

    const QPointF startScene = odomChart->mapToPosition(QPointF(x, y), odomTrailSeries);
    const QPointF tipScene   = odomChart->mapToPosition(tip, odomTrailSeries);

    odomHeadingLine->setLine(QLineF(startScene, tipScene));

    // Cabezal de flecha: triángulo chico calculado en coordenadas de pantalla
    // (no de datos), para que se vea igual de grande sin importar el zoom.
    const double angle     = std::atan2(tipScene.y() - startScene.y(), tipScene.x() - startScene.x());
    const double headLen   = 8.0;
    const double headAngle = qDegreesToRadians(25.0);

    const QPointF p1 = tipScene;
    const QPointF p2 = tipScene - QPointF(headLen * std::cos(angle - headAngle), headLen * std::sin(angle - headAngle));
    const QPointF p3 = tipScene - QPointF(headLen * std::cos(angle + headAngle), headLen * std::sin(angle + headAngle));

    odomHeadingArrowHead->setPolygon(QPolygonF({p1, p2, p3}));
}

void MainWindow::clearOdomMap()
{
    odomTrailSeries->clear();
    odomCurrentSeries->clear();
    odomLostMarkers->clear();
    odomObjMarkers->clear();
    odomBarrierSeries->clear();
    odomObstaclePts->clear();

    for (QLineSeries *seg : qAsConst(odomTapeSegments)) {
        odomChart->removeSeries(seg);
        delete seg;
    }
    odomTapeSegments.clear();
    odomTapeCurrent = nullptr;

    for (const OdomAnnotation &a : qAsConst(odomAnnotations)) {
        odomChart->scene()->removeItem(a.item);
        delete a.item;
    }
    odomAnnotations.clear();

    odomHasPrev          = false;
    odomPrevLineDetected = false;
    odomPrevLineState    = 0;

    odomMinX = -0.5; odomMaxX = 0.5;
    odomMinY = -0.5; odomMaxY = 0.5;
    odomAxisX->setRange(odomMinX, odomMaxX);
    odomAxisY->setRange(odomMinY, odomMaxY);

    odomHeadingLine->setLine(QLineF());
    odomHeadingArrowHead->setPolygon(QPolygonF());

    odomTotalDist = 0.0;
    odomSpeed     = 0.0;
    odomPrevTms   = 0;
    odomMeasureHasP1 = false;
    odomMeasureDone  = false;
    odomMeasureSeries->clear();
    if (odomMeasureText) odomMeasureText->setText("");

    lblOdomInfo->setText("Odometría: esperando datos por WiFi...");
}

void MainWindow::on_pushButton_released() {
    // Acción vacía o algo temporal
}

void MainWindow::processCsvLine(const QByteArray &line)
{
    // Convertir a string
    QString strLine = QString::fromUtf8(line);

    // Ignorar Headers
    if (strLine.startsWith("t_ms") || strLine.startsWith("Time")) {
         return;
    }

    // Separar por comas
    QStringList parts = strLine.split(',');

    // Verificar cantidad de columnas (26)
    if (parts.size() < 26) {
        return; // Línea incompleta o formato incorrecto
    }

    bool ok;
    // 0. t_ms
    telemetryData.t_ms = parts[0].toUInt(&ok);
    if(!ok) return;

    // 1. dt_us
    telemetryData.dt_us = parts[1].toUInt(&ok);

    // 2. dt_ctrl_us
    telemetryData.dt_ctrl_us = parts[2].toUInt(&ok);

    // 3. accel_roll (x1000)
    telemetryData.accel_roll = parts[3].toFloat(&ok) / 1000.0f;

    // 4. accel_roll_f (x1000)
    telemetryData.accel_roll_f = parts[4].toFloat(&ok) / 1000.0f;

    // 5. gyro_y (x1000)
    telemetryData.gyro_y = parts[5].toFloat(&ok) / 1000.0f;

    // 6. gyro_f (x1000)
    telemetryData.gyro_f = parts[6].toFloat(&ok) / 1000.0f;

    // 7. roll_filt (x1000)
    telemetryData.roll_filt = parts[7].toFloat(&ok) / 1000.0f;

    // 8. dyn_sp (x1000)  ← NUEVO
    telemetryData.dyn_sp = parts[8].toFloat(&ok) / 1000.0f;

    // 9. error (x1000)
    telemetryData.error = parts[9].toFloat(&ok) / 1000.0f;

    // 10. p (x1000)
    telemetryData.p = parts[10].toFloat(&ok) / 1000.0f;

    // 11. i (x1000)
    telemetryData.i = parts[11].toFloat(&ok) / 1000.0f;

    // 12. d (x1000)
    telemetryData.d = parts[12].toFloat(&ok) / 1000.0f;

    // 13. output (x1000)
    telemetryData.output = parts[13].toFloat(&ok) / 1000.0f;

    // 14. pwm_cmd (x100) -> Ojo, guia dice x100
    telemetryData.pwm_cmd = parts[14].toFloat(&ok) / 100.0f;

    // 15. pwm_sat (x100) -> Ojo, guia dice x100
    telemetryData.pwm_sat = parts[15].toFloat(&ok) / 100.0f;

    // 16. sat_flag
    telemetryData.sat_flag = (uint8_t)parts[16].toUInt(&ok);

    // 17. mR
    telemetryData.mR = (int16_t)parts[17].toInt(&ok);

    // 18. mL
    telemetryData.mL = (int16_t)parts[18].toInt(&ok);

    // 19. pitch
    telemetryData.pitch = parts[19].toFloat(&ok) / 1000.0f;

    // 20. ax
    telemetryData.ax = parts[20].toFloat(&ok) / 100.0f;
    // 21. ay
    telemetryData.ay = parts[21].toFloat(&ok) / 100.0f;
    // 22. az
    telemetryData.az = parts[22].toFloat(&ok) / 100.0f;
    // 23. gx
    telemetryData.gx = parts[23].toFloat(&ok) / 100.0f;
    // 24. gy
    telemetryData.gy = parts[24].toFloat(&ok) / 100.0f;
    // 25. gz
    telemetryData.gz = parts[25].toFloat(&ok) / 100.0f;

    // Debug opcional para verificar
    // ui->textEdit_PROCCES->append("CSV: " + strLine);
    qDebug() << "CSV Parsed: t=" << telemetryData.t_ms << " Roll=" << telemetryData.roll_filt;

    // --- GRÁFICOS ---
    // Usamos el tiempo recibido (convertido a segundos)
    double t_sec = telemetryData.t_ms / 1000.0;

    // Graficamos Aceleración (accel_roll) en el chart de aceleración
    // Nota: Aunque se llame "accel_roll", lo ponemos en seriesAx como ejemplo
    seriesAx->append(t_sec, telemetryData.accel_roll);

    // Muestro los valores de ROLL y PITCH
    ui->label_inclination_Xvalue->setText(QString::number(telemetryData.roll_filt, 'f', 2) + " °");
    robotViewer3D->setPitch(telemetryData.roll_filt);
    ui->label_inclination_Yvalue->setText(QString::number(telemetryData.pitch, 'f', 2) + " °");

    // Graficamos Giroscopio (gyro_y) en el chart de giroscopio
    seriesGy->append(t_sec, telemetryData.gyro_y);
    seriesGyroF->append(t_sec, telemetryData.gyro_f);

    // Pitch
    seriesPitch->append(t_sec, telemetryData.pitch);
    seriesAccelRollF->append(t_sec, telemetryData.accel_roll_f);

    // Raw Accel
    seriesRawAx->append(t_sec, telemetryData.ax);
    seriesRawAy->append(t_sec, telemetryData.ay);
    seriesRawAz->append(t_sec, telemetryData.az);

    // Raw Gyro
    seriesRawGx->append(t_sec, telemetryData.gx);
    seriesRawGy->append(t_sec, telemetryData.gy);
    seriesRawGz->append(t_sec, telemetryData.gz);

    // Actualizar labels numéricos del tab MPU Crudo
    lblRawAx->setText(QString::number(telemetryData.ax, 'f', 3));
    lblRawAy->setText(QString::number(telemetryData.ay, 'f', 3));
    lblRawAz->setText(QString::number(telemetryData.az, 'f', 3));
    lblRawGx->setText(QString::number(telemetryData.gx, 'f', 2));
    lblRawGy->setText(QString::number(telemetryData.gy, 'f', 2));
    lblRawGz->setText(QString::number(telemetryData.gz, 'f', 2));

    // Actualizamos el eje X para que "corra" con el tiempo
    // Mantenemos una ventana de 10 segundos
    double minX = t_sec - 10.0;
    double maxX = t_sec;
    accAxisX->setRange(minX, maxX);
    gyroAxisX->setRange(minX, maxX);
    rawSensorsAccAxisX->setRange(minX, maxX);
    rawSensorsGyroAxisX->setRange(minX, maxX);

    // --- AUTO-ESCALA EJE Y (Aceleración) ---
    qreal minAcc = 1000.0, maxAcc = -1000.0;
    // Consideramos Ax y AccelRollF para la escala
    for (auto *s : {seriesAx, seriesAccelRollF}) {
        const QList<QPointF> pts = s->points();
        if (!pts.isEmpty()) {
            for (const QPointF &p : pts) {
                if (p.x() >= minX) {
                    if (p.y() < minAcc) minAcc = p.y();
                    if (p.y() > maxAcc) maxAcc = p.y();
                }
            }
        }
    }
    // Agregamos un margen del 10%
    if (maxAcc > minAcc) {
        qreal marginAcc = (maxAcc - minAcc) * 0.1;
        if (marginAcc == 0) marginAcc = 1.0; // Evitar rango nulo
        accAxisY->setRange(minAcc - marginAcc, maxAcc + marginAcc);
    }

    // --- AUTO-ESCALA EJE Y (Giroscopio) ---
    qreal minGyro = 10000.0, maxGyro = -10000.0;
    for (auto *s : {seriesGy, seriesGyroF}) {
        const QList<QPointF> pts = s->points();
        if (!pts.isEmpty()) {
            for (const QPointF &p : pts) {
                if (p.x() >= minX) {
                    if (p.y() < minGyro) minGyro = p.y();
                    if (p.y() > maxGyro) maxGyro = p.y();
                }
            }
        }
    }

    if (maxGyro > minGyro) {
        qreal marginGyro = (maxGyro - minGyro) * 0.1;
        if (marginGyro == 0) marginGyro = 10.0;
        gyroAxisY->setRange(minGyro - marginGyro, maxGyro + marginGyro);
    }

    // --- AUTO-ESCALA RAW ACCEL ---
    qreal minRA = 1000.0, maxRA = -1000.0;
    for(auto *s : {seriesRawAx, seriesRawAy, seriesRawAz}) {
        const QList<QPointF> pts = s->points();
        if(!pts.isEmpty()) {
            for(const QPointF &p : pts) {
                if(p.x() >= minX) {
                    if(p.y() < minRA) minRA = p.y();
                    if(p.y() > maxRA) maxRA = p.y();
                }
            }
        }
    }
    if (maxRA > minRA) {
        qreal m = (maxRA - minRA) * 0.1;
        if (m == 0) m = 1.0;
        rawSensorsAccAxisY->setRange(minRA - m, maxRA + m);
    }

    // --- AUTO-ESCALA RAW GYRO ---
    qreal minRG = 10000.0, maxRG = -10000.0;
    for(auto *s : {seriesRawGx, seriesRawGy, seriesRawGz}) {
        const QList<QPointF> pts = s->points();
        if(!pts.isEmpty()) {
            for(const QPointF &p : pts) {
                if(p.x() >= minX) {
                    if(p.y() < minRG) minRG = p.y();
                    if(p.y() > maxRG) maxRG = p.y();
                }
            }
        }
    }
    if (maxRG > minRG) {
        qreal m = (maxRG - minRG) * 0.1;
        if (m == 0) m = 10.0;
        rawSensorsGyroAxisY->setRange(minRG - m, maxRG + m);
    }

    seriesRollFilt->append(t_sec, telemetryData.roll_filt);
    seriesDynSp->append(t_sec, telemetryData.dyn_sp);

    // --- GRÁFICOS TAB 2: PID ---
    seriesP->append(t_sec, telemetryData.p);
    seriesI->append(t_sec, telemetryData.i);
    seriesD->append(t_sec, telemetryData.d);
    seriesOutput->append(t_sec, telemetryData.output);
    seriesError->append(t_sec, telemetryData.error);

    pidAxisX->setRange(minX, maxX);

    // Auto-escala PID
    qreal minPid = 1000000.0, maxPid = -1000000.0;
    // Chequeamos todos los series del PID
    QList<QLineSeries*> pidSeriesList = {seriesP, seriesI, seriesD, seriesOutput, seriesError};
    for(auto *s : pidSeriesList) {
        const QList<QPointF> points = s->points();
        if(!points.isEmpty()) {
            for(const QPointF &p : points) {
                if(p.x() >= minX) {
                    if(p.y() < minPid) minPid = p.y();
                    if(p.y() > maxPid) maxPid = p.y();
                }
            }
        }
    }
    // Si encontramos valores válidos
    if (minPid != 1000000.0 && maxPid != -1000000.0) {
        qreal margin = (maxPid - minPid) * 0.1;
        if(margin == 0) margin = 0.5;
        pidAxisY->setRange(minPid - margin, maxPid + margin);
    } else {
        pidAxisY->setRange(-1, 1); // Default pequeño si no hay datos
    }

    // --- GRÁFICOS TAB 3: MOTORES ---
    // PWM y Motores (mR, mL) suelen tener escalas similares (-100 a 100)
    seriesPwmCmd->append(t_sec, telemetryData.pwm_cmd);
    seriesPwmSat->append(t_sec, telemetryData.pwm_sat);
    seriesMR->append(t_sec, telemetryData.mR);
    seriesML->append(t_sec, telemetryData.mL);

    motorsAxisX->setRange(minX, maxX);
    motorsAxisY->setRange(-110, 110);

    // --- GRÁFICOS TAB 4: SISTEMA ---
    seriesDt->append(t_sec, telemetryData.dt_us);
    seriesDtCtrl->append(t_sec, telemetryData.dt_ctrl_us);
    // Sat flag es 0 o 1. Lo escalamos para que sea visible en el grafico de dt (que es ~20000)
    // O mejor, usamos ejes separados. Para simplicidad, lo mostramos como 0 o 20000
    double flagVal = (telemetryData.sat_flag > 0) ? 20000.0 : 0.0;
    seriesSatFlag->append(t_sec, flagVal);

    systemAxisX->setRange(minX, maxX);
    systemAxisY->setRange(0, 35000); // dt_us suele andar en 20000-30000

    // --- GRABACIÓN (LOGGING) ---
    if (isRecording && csvLogFile.isOpen()) {
        QTextStream stream(&csvLogFile);
        stream << telemetryData.t_ms << ","
               << telemetryData.dt_us << ","
               << telemetryData.dt_ctrl_us << ","
               << telemetryData.accel_roll << ","
               << telemetryData.accel_roll_f << ","
               << telemetryData.gyro_y << ","
               << telemetryData.gyro_f << ","
               << telemetryData.roll_filt << ","
               << telemetryData.dyn_sp << ","
               << telemetryData.error << ","
               << telemetryData.p << ","
               << telemetryData.i << ","
               << telemetryData.d << ","
               << telemetryData.output << ","
               << telemetryData.pwm_cmd << ","
               << telemetryData.pwm_sat << ","
               << telemetryData.sat_flag << ","
               << telemetryData.mR << ","
               << telemetryData.mL << ","
               << telemetryData.pitch << ","
               << telemetryData.ax << ","
               << telemetryData.ay << ","
               << telemetryData.az << ","
               << telemetryData.gx << ","
               << telemetryData.gy << ","
               << telemetryData.gz << ","
               << 0 << "," // line_error
               << 0 << "," // p_line
               << 0 << "," // i_line
               << 0 << "," // d_line
               << 0 << "," // steering_adjustment
               << 0 << "," // adc1
               << 0 << "," // adc2
               << 0 << "," // adc3
               << 0 << "\n"; // adc4
    }
}

void MainWindow::toggleRecording()
{
    if (!isRecording) {
        // Iniciar grabación
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString filename = "telemetry_" + timestamp + ".csv";

        // Crear carpeta logs explícitamente (si no existe)
        QDir().mkpath(defaultSaveDirectory);

        csvLogFile.setFileName(defaultSaveDirectory + "/" + filename);
        if (csvLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&csvLogFile);
            // Escribir encabezado de acuerdo a la estructura TelemetryData (Updated for 26 columns + 9 Line Follower)
            stream << "t_ms,dt_us,dt_ctrl_us,accel_roll,accel_roll_f,gyro_y,gyro_f,roll_filt,dyn_sp,error,p,i,d,output,pwm_cmd,pwm_sat,sat,mR,mL,pitch,ax,ay,az,gx,gy,gz,line_error,p_line,i_line,d_line,steering_adjustment,adc1,adc2,adc3,adc4\n";

            isRecording = true;
            ui->pushButton_RECORD->setText("⏹ Detener grabación");
            ui->pushButton_RECORD->setChecked(true);

            // Mostrar ruta absoluta para que el usuario sepa dónde está
            QFileInfo fileInfo(csvLogFile);
            ui->textEdit_PROCCES->append("Grabando en: " + fileInfo.absoluteFilePath());
        } else {
            ui->textEdit_PROCCES->append("Error al crear archivo de log: " + csvLogFile.errorString());
            ui->pushButton_RECORD->setChecked(false);
        }
    } else {
        // Detener grabación
        if (csvLogFile.isOpen()) {
            QFileInfo fileInfo(csvLogFile);
            QString baseName = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName();
            csvLogFile.close();
        }
        isRecording = false;
        ui->pushButton_RECORD->setText("⏺ Grabar CSV");
        ui->pushButton_RECORD->setChecked(false);
        ui->textEdit_PROCCES->append("Grabación detenida.");
    }
}

void MainWindow::selectSaveDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Seleccionar Carpeta de Logs"),
                                                    defaultSaveDirectory,
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        defaultSaveDirectory = dir;
        ui->lineEdit_LogDir->setText(defaultSaveDirectory);
    }
}

void MainWindow::on_pushButton_SetKP_clicked()
{
    bool ok = false;
    QString text = ui->lineEdit_KP->text();
    text.replace(',', '.');
    double val = text.toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "El valor de KP no es un número válido.");
        return;
    }

    if (UdpSocket1->state() != QAbstractSocket::BoundState && UdpSocket1->localPort() == 0) {
        QMessageBox::warning(this, "UDP", "Primero abrí el puerto local (Open UDP).");
        return;
    }

    if (clientAddress.isNull() || puertoremoto <= 0) {
        QMessageBox::warning(this, "UDP", "Destino IP o PUERTO no configurado.");
        return;
    }

    _udat w;
    w.f32 = (float)val;
    unsigned char dato[256];
    int indice = 0;

    dato[indice++] = 'U';
    dato[indice++] = 'N';
    dato[indice++] = 'E';
    dato[indice++] = 'R';
    int idxNbytes = indice;
    dato[indice++] = 0x00;
    dato[indice++] = ':';
    int payloadStart = indice;

    dato[indice++] = MODIFYKP;
    dato[indice++] = w.ui8[0];
    dato[indice++] = w.ui8[1];
    dato[indice++] = w.ui8[2];
    dato[indice++] = w.ui8[3];

    unsigned char nbytes = static_cast<unsigned char>(indice - payloadStart + 1);
    dato[idxNbytes] = nbytes;

    unsigned char chk = 'U' ^ 'N' ^ 'E' ^ 'R' ^ dato[idxNbytes] ^ ':';
    for (int i = payloadStart; i < indice; ++i) chk ^= dato[i];
    dato[indice++] = chk;

    int totalLen = 6 + nbytes;
    UdpSocket1->writeDatagram(reinterpret_cast<const char *>(dato), totalLen, clientAddress, static_cast<quint16>(puertoremoto));

    ui->textEdit_PROCCES->append("Set KP (UDP): " + QString::number(val, 'f', 3));
    ui->lineEdit_KP->clear();   // Limpio y dejo pronto
    ui->lineEdit_KP->setPlaceholderText("Nuevo KP");
}

void MainWindow::on_pushButton_SetKD_clicked()
{
    bool ok = false;
    QString text = ui->lineEdit_KD->text();
    text.replace(',', '.');
    double val = text.toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "El valor de KD no es un número válido.");
        return;
    }

    if (UdpSocket1->state() != QAbstractSocket::BoundState && UdpSocket1->localPort() == 0) {
        QMessageBox::warning(this, "UDP", "Primero abrí el puerto local (Open UDP).");
        return;
    }

    if (clientAddress.isNull() || puertoremoto <= 0) {
        QMessageBox::warning(this, "UDP", "Destino IP o PUERTO no configurado.");
        return;
    }

    _udat w;
    w.f32 = (float)val;
    unsigned char dato[256];
    int indice = 0;

    dato[indice++] = 'U';
    dato[indice++] = 'N';
    dato[indice++] = 'E';
    dato[indice++] = 'R';
    int idxNbytes = indice;
    dato[indice++] = 0x00;
    dato[indice++] = ':';
    int payloadStart = indice;

    dato[indice++] = MODIFYKD;
    dato[indice++] = w.ui8[0];
    dato[indice++] = w.ui8[1];
    dato[indice++] = w.ui8[2];
    dato[indice++] = w.ui8[3];

    unsigned char nbytes = static_cast<unsigned char>(indice - payloadStart + 1);
    dato[idxNbytes] = nbytes;

    unsigned char chk = 'U' ^ 'N' ^ 'E' ^ 'R' ^ dato[idxNbytes] ^ ':';
    for (int i = payloadStart; i < indice; ++i) chk ^= dato[i];
    dato[indice++] = chk;

    int totalLen = 6 + nbytes;
    UdpSocket1->writeDatagram(reinterpret_cast<const char *>(dato), totalLen, clientAddress, static_cast<quint16>(puertoremoto));

    ui->textEdit_PROCCES->append("Set KD (UDP): " + QString::number(val, 'f', 3));
    ui->lineEdit_KD->clear();   // Limpio y dejo pronto
    ui->lineEdit_KD->setPlaceholderText("Nuevo KD");
}

void MainWindow::on_pushButton_SetKI_clicked()
{
    bool ok = false;
    QString text = ui->lineEdit_KI->text();
    text.replace(',', '.');
    double val = text.toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "El valor de KI no es un número válido.");
        return;
    }

    if (UdpSocket1->state() != QAbstractSocket::BoundState && UdpSocket1->localPort() == 0) {
        QMessageBox::warning(this, "UDP", "Primero abrí el puerto local (Open UDP).");
        return;
    }

    if (clientAddress.isNull() || puertoremoto <= 0) {
        QMessageBox::warning(this, "UDP", "Destino IP o PUERTO no configurado.");
        return;
    }

    _udat w;
    w.f32 = (float)val;
    unsigned char dato[256];
    int indice = 0;

    dato[indice++] = 'U';
    dato[indice++] = 'N';
    dato[indice++] = 'E';
    dato[indice++] = 'R';
    int idxNbytes = indice;
    dato[indice++] = 0x00;
    dato[indice++] = ':';
    int payloadStart = indice;

    dato[indice++] = MODIFYKI;
    dato[indice++] = w.ui8[0];
    dato[indice++] = w.ui8[1];
    dato[indice++] = w.ui8[2];
    dato[indice++] = w.ui8[3];

    unsigned char nbytes = static_cast<unsigned char>(indice - payloadStart + 1);
    dato[idxNbytes] = nbytes;

    unsigned char chk = 'U' ^ 'N' ^ 'E' ^ 'R' ^ dato[idxNbytes] ^ ':';
    for (int i = payloadStart; i < indice; ++i) chk ^= dato[i];
    dato[indice++] = chk;

    int totalLen = 6 + nbytes;
    UdpSocket1->writeDatagram(reinterpret_cast<const char *>(dato), totalLen, clientAddress, static_cast<quint16>(puertoremoto));

    ui->textEdit_PROCCES->append("Set KI (UDP): " + QString::number(val, 'f', 3));
    ui->lineEdit_KI->clear();   // Limpio y dejo pronto
    ui->lineEdit_KI->setPlaceholderText("Nuevo KI");
}

void MainWindow::on_pushButton_SetKP_LINE_clicked()
{
    bool ok = false;
    QString text = ui->lineEdit_KP_LINE->text();
    text.replace(',', '.');
    double val = text.toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "El valor de KP_LINE no es un número válido.");
        return;
    }

    if (UdpSocket1->state() != QAbstractSocket::BoundState && UdpSocket1->localPort() == 0) {
        QMessageBox::warning(this, "UDP", "Primero abrí el puerto local (Open UDP).");
        return;
    }

    if (clientAddress.isNull() || puertoremoto <= 0) {
        QMessageBox::warning(this, "UDP", "Destino IP o PUERTO no configurado.");
        return;
    }

    _udat w;
    w.f32 = (float)val;
    unsigned char dato[256];
    int indice = 0;

    dato[indice++] = 'U';
    dato[indice++] = 'N';
    dato[indice++] = 'E';
    dato[indice++] = 'R';
    int idxNbytes = indice;
    dato[indice++] = 0x00;
    dato[indice++] = ':';
    int payloadStart = indice;

    dato[indice++] = MODIFY_KP_LINE;
    dato[indice++] = w.ui8[0];
    dato[indice++] = w.ui8[1];
    dato[indice++] = w.ui8[2];
    dato[indice++] = w.ui8[3];

    unsigned char nbytes = static_cast<unsigned char>(indice - payloadStart + 1);
    dato[idxNbytes] = nbytes;

    unsigned char chk = 'U' ^ 'N' ^ 'E' ^ 'R' ^ dato[idxNbytes] ^ ':';
    for (int i = payloadStart; i < indice; ++i) chk ^= dato[i];
    dato[indice++] = chk;

    int totalLen = 6 + nbytes;
    UdpSocket1->writeDatagram(reinterpret_cast<const char *>(dato), totalLen, clientAddress, static_cast<quint16>(puertoremoto));

    ui->textEdit_PROCCES->append("Set KP_LINE (UDP): " + QString::number(val, 'f', 3));
    ui->label_KP_LINE->setText(QString::number(val, 'f', 3));
    ui->lineEdit_KP_LINE->clear();
    ui->lineEdit_KP_LINE->setPlaceholderText("Nuevo KP");
}

void MainWindow::on_pushButton_SetKD_LINE_clicked()
{
    bool ok = false;
    QString text = ui->lineEdit_KD_LINE->text();
    text.replace(',', '.');
    double val = text.toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "El valor de KD_LINE no es un número válido.");
        return;
    }

    if (UdpSocket1->state() != QAbstractSocket::BoundState && UdpSocket1->localPort() == 0) {
        QMessageBox::warning(this, "UDP", "Primero abrí el puerto local (Open UDP).");
        return;
    }

    if (clientAddress.isNull() || puertoremoto <= 0) {
        QMessageBox::warning(this, "UDP", "Destino IP o PUERTO no configurado.");
        return;
    }

    _udat w;
    w.f32 = (float)val;
    unsigned char dato[256];
    int indice = 0;

    dato[indice++] = 'U';
    dato[indice++] = 'N';
    dato[indice++] = 'E';
    dato[indice++] = 'R';
    int idxNbytes = indice;
    dato[indice++] = 0x00;
    dato[indice++] = ':';
    int payloadStart = indice;

    dato[indice++] = MODIFY_KD_LINE;
    dato[indice++] = w.ui8[0];
    dato[indice++] = w.ui8[1];
    dato[indice++] = w.ui8[2];
    dato[indice++] = w.ui8[3];

    unsigned char nbytes = static_cast<unsigned char>(indice - payloadStart + 1);
    dato[idxNbytes] = nbytes;

    unsigned char chk = 'U' ^ 'N' ^ 'E' ^ 'R' ^ dato[idxNbytes] ^ ':';
    for (int i = payloadStart; i < indice; ++i) chk ^= dato[i];
    dato[indice++] = chk;

    int totalLen = 6 + nbytes;
    UdpSocket1->writeDatagram(reinterpret_cast<const char *>(dato), totalLen, clientAddress, static_cast<quint16>(puertoremoto));

    ui->textEdit_PROCCES->append("Set KD_LINE (UDP): " + QString::number(val, 'f', 3));
    ui->label_KD_LINE->setText(QString::number(val, 'f', 3));
    ui->lineEdit_KD_LINE->clear();
    ui->lineEdit_KD_LINE->setPlaceholderText("Nuevo KD");
}

void MainWindow::on_pushButton_SetKI_LINE_clicked()
{
    bool ok = false;
    QString text = ui->lineEdit_KI_LINE->text();
    text.replace(',', '.');
    double val = text.toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "El valor de KI_LINE no es un número válido.");
        return;
    }

    if (UdpSocket1->state() != QAbstractSocket::BoundState && UdpSocket1->localPort() == 0) {
        QMessageBox::warning(this, "UDP", "Primero abrí el puerto local (Open UDP).");
        return;
    }

    if (clientAddress.isNull() || puertoremoto <= 0) {
        QMessageBox::warning(this, "UDP", "Destino IP o PUERTO no configurado.");
        return;
    }

    _udat w;
    w.f32 = (float)val;
    unsigned char dato[256];
    int indice = 0;

    dato[indice++] = 'U';
    dato[indice++] = 'N';
    dato[indice++] = 'E';
    dato[indice++] = 'R';
    int idxNbytes = indice;
    dato[indice++] = 0x00;
    dato[indice++] = ':';
    int payloadStart = indice;

    dato[indice++] = MODIFY_KI_LINE;
    dato[indice++] = w.ui8[0];
    dato[indice++] = w.ui8[1];
    dato[indice++] = w.ui8[2];
    dato[indice++] = w.ui8[3];

    unsigned char nbytes = static_cast<unsigned char>(indice - payloadStart + 1);
    dato[idxNbytes] = nbytes;

    unsigned char chk = 'U' ^ 'N' ^ 'E' ^ 'R' ^ dato[idxNbytes] ^ ':';
    for (int i = payloadStart; i < indice; ++i) chk ^= dato[i];
    dato[indice++] = chk;

    int totalLen = 6 + nbytes;
    UdpSocket1->writeDatagram(reinterpret_cast<const char *>(dato), totalLen, clientAddress, static_cast<quint16>(puertoremoto));

    ui->textEdit_PROCCES->append("Set KI_LINE (UDP): " + QString::number(val, 'f', 3));
    ui->label_KI_LINE->setText(QString::number(val, 'f', 3));
    ui->lineEdit_KI_LINE->clear();
    ui->lineEdit_KI_LINE->setPlaceholderText("Nuevo KI");
}

void MainWindow::sendManualCommand()
{
    // Verificar si el socket está listo para enviar
    if (UdpSocket1->state() != QAbstractSocket::BoundState && UdpSocket1->localPort() == 0) {
        return;
    }
    if (clientAddress.isNull() || puertoremoto <= 0) {
        return;
    }

    if (currentManualCommand == 0) return;

    unsigned char dato[256];
    int indice = 0;

    dato[indice++] = 'U';
    dato[indice++] = 'N';
    dato[indice++] = 'E';
    dato[indice++] = 'R';
    int idxNbytes = indice;
    dato[indice++] = 0x00;
    dato[indice++] = ':';
    int payloadStart = indice;

    dato[indice++] = currentManualCommand;

    unsigned char nbytes = static_cast<unsigned char>(indice - payloadStart + 1);
    dato[idxNbytes] = nbytes;

    unsigned char chk = 'U' ^ 'N' ^ 'E' ^ 'R' ^ dato[idxNbytes] ^ ':';
    for (int i = payloadStart; i < indice; ++i) chk ^= dato[i];
    dato[indice++] = chk;

    int totalLen = 6 + nbytes;
    UdpSocket1->writeDatagram(reinterpret_cast<const char *>(dato), totalLen, clientAddress, static_cast<quint16>(puertoremoto));
}

void MainWindow::sendAlivePing()
{
    // Ping de latencia: manda GETALIVE directo por UDP (mismo patrón que
    // sendManualCommand, sin pasar por comboBox_CMD) y arranca un cronómetro; el
    // RTT se mide cuando llega el ACK en decodeData() (case GETALIVE).
    if (UdpSocket1->state() != QAbstractSocket::BoundState && UdpSocket1->localPort() == 0) {
        return;
    }
    if (clientAddress.isNull() || puertoremoto <= 0) {
        return;
    }

    unsigned char dato[8];
    int indice = 0;

    dato[indice++] = 'U';
    dato[indice++] = 'N';
    dato[indice++] = 'E';
    dato[indice++] = 'R';
    int idxNbytes = indice;
    dato[indice++] = 0x00;
    dato[indice++] = ':';
    int payloadStart = indice;

    dato[indice++] = GETALIVE;

    unsigned char nbytes = static_cast<unsigned char>(indice - payloadStart + 1);
    dato[idxNbytes] = nbytes;

    unsigned char chk = 'U' ^ 'N' ^ 'E' ^ 'R' ^ dato[idxNbytes] ^ ':';
    for (int i = payloadStart; i < indice; ++i) chk ^= dato[i];
    dato[indice++] = chk;

    aliveSentTimer.start();

    int totalLen = 6 + nbytes;
    UdpSocket1->writeDatagram(reinterpret_cast<const char *>(dato), totalLen, clientAddress, static_cast<quint16>(puertoremoto));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    bool manualCmdTriggered = false;
    switch (event->key()) {
    case Qt::Key_Up:
        currentManualCommand = MOVE_FORWARD;
        manualCmdTriggered = true;
        break;
    case Qt::Key_Down:
        currentManualCommand = MOVE_BACKWARD;
        manualCmdTriggered = true;
        break;
    case Qt::Key_Left:
        currentManualCommand = MOVE_LEFT;
        manualCmdTriggered = true;
        break;
    case Qt::Key_Right:
        currentManualCommand = MOVE_RIGHT;
        manualCmdTriggered = true;
        break;
    default:
        break;
    }

    if (manualCmdTriggered) {
        sendManualCommand(); // Enviar el primer comando inmediatamente
        if (!manualControlTimer->isActive()) {
            manualControlTimer->start(50); // Enviar cada 50 ms (20Hz)
        }
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        QMainWindow::keyReleaseEvent(event);
        return;
    }

    bool manualCmdReleased = false;
    switch (event->key()) {
    case Qt::Key_Up:
        if (currentManualCommand == MOVE_FORWARD) manualCmdReleased = true;
        break;
    case Qt::Key_Down:
        if (currentManualCommand == MOVE_BACKWARD) manualCmdReleased = true;
        break;
    case Qt::Key_Left:
        if (currentManualCommand == MOVE_LEFT) manualCmdReleased = true;
        break;
    case Qt::Key_Right:
        if (currentManualCommand == MOVE_RIGHT) manualCmdReleased = true;
        break;
    default:
        break;
    }

    if (manualCmdReleased) {
        manualControlTimer->stop();
        currentManualCommand = MOVE_STOP;
        sendManualCommand(); // Enviar comando STOP
        currentManualCommand = 0; // Limpiar
    } else {
        QMainWindow::keyReleaseEvent(event);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (!keyEvent->isAutoRepeat()) {
            bool handled = false;
            switch (keyEvent->key()) {
            case Qt::Key_Up:
                currentManualCommand = MOVE_FORWARD;
                handled = true; break;
            case Qt::Key_Down:
                currentManualCommand = MOVE_BACKWARD;
                handled = true; break;
            case Qt::Key_Left:
                currentManualCommand = MOVE_LEFT;
                handled = true; break;
            case Qt::Key_Right:
                currentManualCommand = MOVE_RIGHT;
                handled = true; break;
            default: break;
            }
            if (handled) {
                // --- LOG al cambiar comando ---
                QString cmdName;
                switch (currentManualCommand) {
                case MOVE_FORWARD:  cmdName = "ADELANTE ↑";  break;
                    case MOVE_BACKWARD: cmdName = "ATRÁS ↓";     break;
                    case MOVE_LEFT:     cmdName = "IZQUIERDA ←"; break;
                    case MOVE_RIGHT:    cmdName = "DERECHA →";   break;
                        default:            cmdName = "DESCONOCIDO"; break;
                }
                ui->textEdit_PROCCES->append("MANUAL CMD: " + cmdName
                                             + " [0x" + QString("%1").arg(currentManualCommand, 2, 16, QChar('0')).toUpper() + "]");

                sendManualCommand();
                if (!manualControlTimer->isActive())
                    manualControlTimer->start(50);
                return true;
            }
        }
    }
    else if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (!keyEvent->isAutoRepeat()) {
            bool handled = false;
            switch (keyEvent->key()) {
            case Qt::Key_Up:
                if (currentManualCommand == MOVE_FORWARD) handled = true;
                break;
            case Qt::Key_Down:
                if (currentManualCommand == MOVE_BACKWARD) handled = true;
                break;
            case Qt::Key_Left:
                if (currentManualCommand == MOVE_LEFT) handled = true;
                break;
            case Qt::Key_Right:
                if (currentManualCommand == MOVE_RIGHT) handled = true;
                break;
            default:
                break;
            }
            if (handled) {
                manualControlTimer->stop();
                currentManualCommand = MOVE_STOP;

                // --- LOG al soltar (STOP) ---
                ui->textEdit_PROCCES->append("MANUAL CMD: STOP ■"
                                             + QString(" [0x%1]").arg(MOVE_STOP, 2, 16, QChar('0')).toUpper());

                sendManualCommand();
                currentManualCommand = 0;
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::clearUdpScreens()
{
    resetConsoleHeaders(ui->textEdit_RAW, ui->textEdit_PROCCES);
}

void MainWindow::checkUdpInactivity()
{
    if (!udpEverReceivedData) {
        return; // Nunca recibimos nada aún, no disparamos el watchdog
    }

    // 25000 ms = 25 segundos
    if (udpLastRxTime.elapsed() > 25000) {
        clearUdpScreens();
        ui->lineEdit_IP_REMOTA->clear();
        ui->lineEdit_DEVICEPORT->clear();

        ui->spinBox_SETPOINT->setValue(0.0);
        ui->textEdit_PROCCES->append("WATCHDOG UDP: Timeout (15s sin datos). Reiniciando socket...");

        // Obtener el puerto local
        bool ok = false;
        const int localPort = ui->lineEdit_LOCALPORT->text().toInt(&ok, 10);
        if (!ok || localPort <= 0 || localPort > 65535) {
            ui->textEdit_PROCCES->append("WATCHDOG ERROR: Puerto local inválido para rebind.");
            udpWatchdogTimer->stop();
            return;
        }

        // 1. Cerrar el socket UDP actual
        UdpSocket1->abort();

        // 2. Volver a abrirlo haciendo bind sobre el mismo puerto local
        const bool okBind = UdpSocket1->bind(
            QHostAddress::AnyIPv4,
            static_cast<quint16>(localPort),
            QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
        );

        if (!okBind) {
            ui->textEdit_PROCCES->append(QString("WATCHDOG ERROR: Falló el rebind al puerto %1: %2").arg(localPort).arg(UdpSocket1->errorString()));
            // Mantenemos el estado activo visualmente (rojo para cerrar) para que el usuario sepa
            // que la app "cree" que debe estar abierto, o al menos no rompemos el toggle manual.
        } else {
            ui->textEdit_PROCCES->append("WATCHDOG UDP: Socket reabierto correctamente. Escuchando...");
            // Queda escuchando nuevamente. No reiniciamos la bandera "udpEverReceivedData" para que si sigue sin haber datos, a los 15s vuelva a reconectar?
            // "No quiero un loop de cerrar/abrir si nunca llegó nada desde que abrí el puerto."
            // Ah, el usuario dice: "Solo debe dispararse el watchdog si antes ya se habían recibido datos UDP al menos una vez."
            // Para evitar un loop infinito de cerrar/abrir, reseteamos la bandera.
            // Así esperará un nuevo paquete antes de iniciar el timeout de 15s de nuevo.
            udpEverReceivedData = false;
        }
    }
}

void MainWindow::on_pushButton_SETPOINT_clicked()
{
    // Seleccionar MODIFY_SETPOINT en el comboBox
    int idx = ui->comboBox_CMD->findData(MODIFY_SETPOINT);
    if (idx >= 0) ui->comboBox_CMD->setCurrentIndex(idx);

    // Enviar por el canal disponible
    if (serial->isOpen()) {
        sendDataSerial();
    } else if (UdpSocket1->state() == QAbstractSocket::BoundState
               && !clientAddress.isNull() && puertoremoto > 0) {
        sendDataUDP();
    } else {
        ui->textEdit_PROCCES->append("SET SETPOINT: Sin conexión disponible.");
    }
}
