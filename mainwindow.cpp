#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork/QNetworkDatagram>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    serial=new QSerialPort(this);
    settingPorts=new SettingsDialog(this);
    estadoSerial = new QLabel(this);
    estadoSerial->setText("Desconectado ......");
    ui->statusbar->addWidget(estadoSerial);
    ui->actionDisconnect_Device->setEnabled(false);
    timer1=new QTimer(this);
    UdpSocket1 = new QUdpSocket(this);

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

    ui->comboBox_CMD->addItem("ALIVE", 0xF0);
    ui->comboBox_CMD->addItem("FIRMWARE", 0xF1);
    ui->comboBox_CMD->addItem("GETADCVALUES", 0xA5);
    ui->comboBox_CMD->addItem("MPU6050", 0xA6);
    ui->comboBox_CMD->addItem("ANGLE", 0xA7);
    ui->comboBox_CMD->addItem("MOTORES", 0xA1);
    ui->comboBox_CMD->addItem("VELOCIDAD", 0xA4);
    ui->comboBox_CMD->addItem("SENDALLSENSORS", 0xA9);
    ui->comboBox_CMD->addItem("MODIFYKP", 0xB1);
    ui->comboBox_CMD->addItem("MODIFYKD", 0xB2);
    ui->comboBox_CMD->addItem("MODIFYKI", 0xB3);
    ui->comboBox_CMD->addItem("BALANCE", 0xB4);
    ui->comboBox_CMD->addItem("RESETMASSCENTER", 0xB7);

    estadoProtocolo=START;
    estadoProtocoloUdp = START;
    rxData.timeOut=0;
    ui->pushButton_UDP->setEnabled(false);
    ui->pushButton_ALIVE->setEnabled(false);
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

    seriesGx = new QLineSeries(); seriesGx->setName("Giroscopio Y");
    seriesGx->setColor(Qt::blue);

    seriesGy = new QLineSeries(); seriesGy->setName("Gyr Y (reservado)");
    seriesGy->setColor(Qt::gray);
    seriesGz = new QLineSeries(); seriesGz->setName("Gyr Z (reservado)");
    seriesGz->setColor(Qt::lightGray);

    // 4) Añade sólo las series de aceleración al accChart
    accChart->addSeries(seriesAx);
    accChart->addSeries(seriesRollFilt); // Agregamos el filtrado

    accChart->legend()->setVisible(true);
    accChart->legend()->setAlignment(Qt::AlignBottom);

    //    y las de giroscopio al gyroChart
    // El CSV trae gyro_y, usaremos la serie "seriesGx" (o renombramos variables, pero para minimizar cambios usamos seriesGx como principal)
    // Corrección: El código anterior usaba seriesGy para gyro_y. Mantengamos eso.
    seriesGy->setName("Giroscopio Y");
    seriesGy->setColor(Qt::blue);

    // gyroChart->addSeries(seriesGx);
    gyroChart->addSeries(seriesGy);
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

    seriesGy->attachAxis(gyroAxisX);
    seriesGy->attachAxis(gyroAxisY);

    // --- TAB 2: PID ---
    pidChartView = ui->pidWidget;
    pidChart = new QChart();
    pidChartView->setChart(pidChart);
    pidChartView->setRenderHint(QPainter::Antialiasing);
    pidChart->setTitle("PID Control");

    seriesP = new QLineSeries(); seriesP->setName("Proporcional"); seriesP->setColor(Qt::red);
    seriesI = new QLineSeries(); seriesI->setName("Integral"); seriesI->setColor(Qt::green);
    seriesD = new QLineSeries(); seriesD->setName("Derivativo"); seriesD->setColor(Qt::blue);
    seriesOutput = new QLineSeries(); seriesOutput->setName("Output"); seriesOutput->setColor(Qt::black);
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

    systemChart->addSeries(seriesDt);
    systemChart->addSeries(seriesSatFlag);

    systemAxisX = new QValueAxis(); systemAxisX->setTitleText("Tiempo (s)");
    systemAxisY = new QValueAxis(); systemAxisY->setTitleText("Valor");
    systemChart->addAxis(systemAxisX, Qt::AlignBottom);
    systemChart->addAxis(systemAxisY, Qt::AlignLeft);

    seriesDt->attachAxis(systemAxisX);
    seriesDt->attachAxis(systemAxisY);
    seriesSatFlag->attachAxis(systemAxisX);
    seriesSatFlag->attachAxis(systemAxisY);

    systemChart->legend()->setVisible(true);
    systemChart->legend()->setAlignment(Qt::AlignBottom);

    // 7) Iniciar el tiempo
    elapsedSec = 0;
    mpuPollTimer = new QTimer(this);
    connect(mpuPollTimer, &QTimer::timeout, this, &MainWindow::updatePlot);
    mpuStarted    = false;

    // 1) Asocia tu widget promovido
    adcChartView = ui->adcBarrasWidget;

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
        ui->pushButton_ALIVE->setEnabled(true);
        ui->pushButton_SEND->setEnabled(true);
        ui->actionDisconnect_Device->setEnabled(true);
        estadoSerial->setStyleSheet("QLabel { color : blue; }");
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

void MainWindow::on_pushButton_ALIVE_clicked()
{
    ui->comboBox_CMD->setCurrentIndex(0);
    sendDataSerial();
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
        ui->pushButton_OPENUDP->setText("Open UDP");
        ui->pushButton_UDP->setEnabled(false);
        ui->lineEdit_IP_REMOTA->clear();
        ui->lineEdit_DEVICEPORT->clear();
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

    ui->pushButton_OPENUDP->setText("Close UDP");
    ui->pushButton_UDP->setEnabled(true);

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
        QNetworkDatagram dgram = UdpSocket1->receiveDatagram();
        const QByteArray payload = dgram.data();
        RemoteAddress = dgram.senderAddress();
        RemotePort    = dgram.senderPort();

        // Aprender/actualizar IP/puerto destino (para respuestas)
        clientAddress = RemoteAddress;
        puertoremoto  = RemotePort;

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

        double kp_val = QInputDialog::getDouble(this, "Factor KP", "KP:", 0.0, 0.0, 1000.0, 2, &ok);
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

        double kd_val = QInputDialog::getDouble(this, "Factor KD", "KD:", 0.0, 0.0, 1000.0, 2, &ok);
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

        double ki_val = QInputDialog::getDouble(this, "Factor KI", "KI:", 0.0, 0.0, 1000.0, 2, &ok);
        if (!ok) return;
        w.f32 = (float)ki_val;
        dato[indice++] = w.ui8[0];
        dato[indice++] = w.ui8[1];
        dato[indice++] = w.ui8[2];
        dato[indice++] = w.ui8[3];
        break;
    }

        // Comandos simples (sólo ID)
    case GETALIVE:          // 0xF0
    case GETANGLE:          // 0xA7
    case GETADCVALUES:      // 0xA5
    case GETMPU6050VALUES:  // 0xA6
    case SENDALLSENSORS:    // 0xA9
    case GETDISTANCE:       // 0xA3
    case GETSPEED:          // 0xA4
    case GETSWITCHES:       // ¡revisar si no choca con 0xA5!
    case GETFIRMWARE:       // 0xF1
    case GETANALOGSENSORS:  // 0xA0
    case BALANCE: //BALANCE=0xB4
    case RESETMASSCENTER:   // RESETMASSCENTER=0xB7
    case SETLEDS:
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

        double kp_val = QInputDialog::getDouble(this, "Factor KP", "KP:", 0.0, 0.0, 1000.0, 2, &ok);
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

        double kd_val = QInputDialog::getDouble(this, "Factor KD", "KD:", 0.0, 0.0, 1000.0, 2, &ok);
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

        double ki_val = QInputDialog::getDouble(this, "Factor KI", "KI:", 0.0, 0.0, 1000.0, 2, &ok);
        if (!ok) return;
        w.f32 = (float)ki_val;
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
    case SETLEDS:
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
            } else {
                    contadorAlive++;
                    str="ALIVE BLUEPILL VIA *UDP* RECIBIDO N°: " + QString().number(contadorAlive,10);
            }
        } else {
            str= "ALIVE BLUEPILL VIA *SERIE*  NO ACK!!!";
        }
        ui->textEdit_PROCCES->append(str);
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
        w.ui8[0] = datosRx[2];  // ADC1
        w.ui8[1] = datosRx[3];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_1->setText(str);
        adcValues[0] = w.ui16[0];

        w.ui8[0] = datosRx[4];  // ADC2
        w.ui8[1] = datosRx[5];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_2->setText(str);
        adcValues[1] = w.ui16[0];

        w.ui8[0] = datosRx[6];  // ADC3
        w.ui8[1] = datosRx[7];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_3->setText(str);
        adcValues[2] = w.ui16[0];

        w.ui8[0] = datosRx[8];  // ADC4
        w.ui8[1] = datosRx[9];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_4->setText(str);
        adcValues[3] = w.ui16[0];

        w.ui8[0] = datosRx[10];  // ADC5
        w.ui8[1] = datosRx[11];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_5->setText(str);
        adcValues[4] = w.ui16[0];

        w.ui8[0] = datosRx[12];  // ADC6
        w.ui8[1] = datosRx[13];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_6->setText(str);
        adcValues[5] = w.ui16[0];

        w.ui8[0] = datosRx[14];  // ADC7
        w.ui8[1] = datosRx[15];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_7->setText(str);
        adcValues[6] = w.ui16[0];

        w.ui8[0] = datosRx[16];  // ADC8
        w.ui8[1] = datosRx[17];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_8->setText(str);
        adcValues[7] = w.ui16[0];

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
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_AX_VALUE->setText(str);
        lastAx= w.i16[0];

        w.ui8[0] = datosRx[4];  // AY
        w.ui8[1] = datosRx[5];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_AY_VALUE->setText(str);
        lastAy = w.i16[0];

        w.ui8[0] = datosRx[6];  // AZ
        w.ui8[1] = datosRx[7];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_AZ_VALUE->setText(str);

        w.ui8[0] = datosRx[8];  // GX
        w.ui8[1] = datosRx[9];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_GX_VALUE->setText(str);

        w.ui8[0] = datosRx[10];  // GY
        w.ui8[1] = datosRx[11];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_GY_VALUE->setText(str);

        w.ui8[0] = datosRx[12];  // GZ
        w.ui8[1] = datosRx[13];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_GZ_VALUE->setText(str);

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

    case MODIFYKP: { //    MODIFYKP=0xB1,
        w.ui8[0] = datosRx[2];  // KP VALUE - Byte 0
        w.ui8[1] = datosRx[3];  // KP VALUE - Byte 1
        w.ui8[2] = datosRx[4];  // KP VALUE - Byte 2
        w.ui8[3] = datosRx[5];  // KP VALUE - Byte 3
        float KP_Value = w.f32; // <--- CORREGIDO (sin [0])
        QString strKP = QString::number(KP_Value, 'f', 2); // Muestra con 2 decimales
        ui->label_KP->setText(strKP);

        w.ui8[0] = datosRx[6];  // KD VALUE - Byte 0
        w.ui8[1] = datosRx[7];  // KD VALUE - Byte 1
        w.ui8[2] = datosRx[8];  // KD VALUE - Byte 2
        w.ui8[3] = datosRx[9];  // KD VALUE - Byte 3
        float KD_Value = w.f32; // <--- CORREGIDO (sin [0])
        QString strKD = QString::number(KD_Value, 'f', 2); // Muestra con 2 decimales
        ui->label_KD->setText(strKD);

        w.ui8[0] = datosRx[10]; // KI VALUE - Byte 0
        w.ui8[1] = datosRx[11]; // KI VALUE - Byte 1
        w.ui8[2] = datosRx[12]; // KI VALUE - Byte 2
        w.ui8[3] = datosRx[13]; // KI VALUE - Byte 3
        float KI_Value = w.f32; // <--- CORREGIDO (sin [0])
        QString strKI = QString::number(KI_Value, 'f', 2); // Muestra con 2 decimales
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
        QString strKP = QString::number(KP_Value, 'f', 2); // Muestra con 2 decimales
        ui->label_KP->setText(strKP);

        w.ui8[0] = datosRx[6];  // KD VALUE - Byte 0
        w.ui8[1] = datosRx[7];  // KD VALUE - Byte 1
        w.ui8[2] = datosRx[8];  // KD VALUE - Byte 2
        w.ui8[3] = datosRx[9];  // KD VALUE - Byte 3
        float KD_Value = w.f32; // <--- CORREGIDO
        QString strKD = QString::number(KD_Value, 'f', 2); // Muestra con 2 decimales
        ui->label_KD->setText(strKD);

        w.ui8[0] = datosRx[10]; // KI VALUE - Byte 0
        w.ui8[1] = datosRx[11]; // KI VALUE - Byte 1
        w.ui8[2] = datosRx[12]; // KI VALUE - Byte 2
        w.ui8[3] = datosRx[13]; // KI VALUE - Byte 3
        float KI_Value = w.f32; // <--- CORREGIDO
        QString strKI = QString::number(KI_Value, 'f', 2); // Muestra con 2 decimales
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
        QString strKP = QString::number(KP_Value, 'f', 2); // Muestra con 2 decimales
        ui->label_KP->setText(strKP);

        w.ui8[0] = datosRx[6];  // KD VALUE - Byte 0
        w.ui8[1] = datosRx[7];  // KD VALUE - Byte 1
        w.ui8[2] = datosRx[8];  // KD VALUE - Byte 2
        w.ui8[3] = datosRx[9];  // KD VALUE - Byte 3
        float KD_Value = w.f32; // <--- CORREGIDO
        QString strKD = QString::number(KD_Value, 'f', 2); // Muestra con 2 decimales
        ui->label_KD->setText(strKD);

        w.ui8[0] = datosRx[10]; // KI VALUE - Byte 0
        w.ui8[1] = datosRx[11]; // KI VALUE - Byte 1
        w.ui8[2] = datosRx[12]; // KI VALUE - Byte 2
        w.ui8[3] = datosRx[13]; // KI VALUE - Byte 3
        float KI_Value = w.f32; // <--- CORREGIDO
        QString strKI = QString::number(KI_Value, 'f', 2); // Muestra con 2 decimales
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

        w.ui8[0] = datosRx[2];  // ADC1
        w.ui8[1] = datosRx[3];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_1->setText(str);
        adcValues[0] = w.ui16[0];

        w.ui8[0] = datosRx[4];  // ADC2
        w.ui8[1] = datosRx[5];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_2->setText(str);
        adcValues[1] = w.ui16[0];

        w.ui8[0] = datosRx[6];  // ADC3
        w.ui8[1] = datosRx[7];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_3->setText(str);
        adcValues[2] = w.ui16[0];

        w.ui8[0] = datosRx[8];  // ADC4
        w.ui8[1] = datosRx[9];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_4->setText(str);
        adcValues[3] = w.ui16[0];

        w.ui8[0] = datosRx[10];  // ADC5
        w.ui8[1] = datosRx[11];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_5->setText(str);
        adcValues[4] = w.ui16[0];

        w.ui8[0] = datosRx[12];  // ADC6
        w.ui8[1] = datosRx[13];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_6->setText(str);
        adcValues[5] = w.ui16[0];

        w.ui8[0] = datosRx[14];  // ADC7
        w.ui8[1] = datosRx[15];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_7->setText(str);
        adcValues[6] = w.ui16[0];

        w.ui8[0] = datosRx[16];  // ADC8
        w.ui8[1] = datosRx[17];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_ADCVALUE_8->setText(str);
        adcValues[7] = w.ui16[0];

        w.ui8[0] = datosRx[18];  // AX
        w.ui8[1] = datosRx[19];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_AX_VALUE->setText(str);
        ax = w.i16[0];
        lastAx = w.i16[0];

        w.ui8[0] = datosRx[20];  // AY
        w.ui8[1] = datosRx[21];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_AY_VALUE->setText(str);
        ay = w.i16[0];
        lastAy = w.i16[0];

        w.ui8[0] = datosRx[22];  // AZ
        w.ui8[1] = datosRx[23];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_AZ_VALUE->setText(str);
        az = w.i16[0];

        w.ui8[0] = datosRx[24];  // GX
        w.ui8[1] = datosRx[25];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_GX_VALUE->setText(str);
        gx = w.i16[0];

        w.ui8[0] = datosRx[26];  // GY
        w.ui8[1] = datosRx[27];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_GY_VALUE->setText(str);
        gy = w.i16[0];

        w.ui8[0] = datosRx[28];  // GZ
        w.ui8[1] = datosRx[29];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        ui->label_GZ_VALUE->setText(str);
        gz = w.i16[0];

        // ROLL
        w.ui8[0] = datosRx[30];
        w.ui8[1] = datosRx[31];
        qint16 roll = w.i16[0];  // firmado
        QString strRoll = QString::number(roll) + " \u00B0";
        ui->label_inclination_Xvalue->setText(strRoll);

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
        }
        ui->textEdit_PROCCES->append(str);
        break;
    case RESETMASSCENTER:
        if(datosRx[2]==ACK){
            str="Se ha reestablecido el punto de balance correctamente!";
        }
        ui->textEdit_PROCCES->append(str);
        break;
    default:
        str = str + "Comando DESCONOCIDO!!!!";
        ui->textEdit_PROCCES->append(str);
    }
}

void MainWindow::on_pushButton_clicked()    // BOTON DE "CLEAR DISPLAYS"
{
    ui->textEdit_PROCCES->clear();
    ui->textEdit_RAW->clear();
    ui->textEdit_RAW->append("        Dato sin procesar");
    ui->textEdit_PROCCES->append("      Dato procesado");
}

void MainWindow::updatePlot() {
    // 1) Desplaza la ventana de tiempo en ambos ejes X
    accAxisX->setRange(elapsedSec - 10.0, elapsedSec);
    gyroAxisX->setRange(elapsedSec - 10.0, elapsedSec);

    // 2) Calcula mínimo y máximo sólo para las series de aceleración
    qreal minAcc = std::numeric_limits<qreal>::max();
    qreal maxAcc = std::numeric_limits<qreal>::lowest();
    for (auto *s : {seriesAx, seriesAy, seriesAz}) {
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
    for (auto *s : {seriesGx, seriesGy, seriesGz}) {
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

    // Verificar cantidad de columnas (15)
    if (parts.size() < 15) {
        return; // Línea incompleta o formato incorrecto
    }

    bool ok;
    // 0. t_ms
    telemetryData.t_ms = parts[0].toUInt(&ok);
    if(!ok) return;

    // 1. dt_us
    telemetryData.dt_us = parts[1].toUInt(&ok);

    // 2. accel_roll (x1000)
    telemetryData.accel_roll = parts[2].toFloat(&ok) / 1000.0f;

    // 3. gyro_y (x1000)
    telemetryData.gyro_y = parts[3].toFloat(&ok) / 1000.0f;

    // 4. roll_filt (x1000)
    telemetryData.roll_filt = parts[4].toFloat(&ok) / 1000.0f;

    // 5. error (x1000)
    telemetryData.error = parts[5].toFloat(&ok) / 1000.0f;

    // 6. p (x1000)
    telemetryData.p = parts[6].toFloat(&ok) / 1000.0f;

    // 7. i (x1000)
    telemetryData.i = parts[7].toFloat(&ok) / 1000.0f;

    // 8. d (x1000)
    telemetryData.d = parts[8].toFloat(&ok) / 1000.0f;

    // 9. output (x1000)
    telemetryData.output = parts[9].toFloat(&ok) / 1000.0f;

    // 10. pwm_cmd (x100) -> Ojo, guia dice x100
    telemetryData.pwm_cmd = parts[10].toFloat(&ok) / 100.0f;

    // 11. pwm_sat (x100) -> Ojo, guia dice x100
    telemetryData.pwm_sat = parts[11].toFloat(&ok) / 100.0f;

    // 12. sat_flag
    telemetryData.sat_flag = (uint8_t)parts[12].toUInt(&ok);

    // 13. mR
    telemetryData.mR = (int16_t)parts[13].toInt(&ok);

    // 14. mL
    telemetryData.mL = (int16_t)parts[14].toInt(&ok);

    // Debug opcional para verificar
    // ui->textEdit_PROCCES->append("CSV: " + strLine);
    qDebug() << "CSV Parsed: t=" << telemetryData.t_ms << " Roll=" << telemetryData.roll_filt;

    // --- GRÁFICOS ---
    // Usamos el tiempo recibido (convertido a segundos)
    double t_sec = telemetryData.t_ms / 1000.0;

    // Graficamos Aceleración (accel_roll) en el chart de aceleración
    // Nota: Aunque se llame "accel_roll", lo ponemos en seriesAx como ejemplo
    seriesAx->append(t_sec, telemetryData.accel_roll);

    // Graficamos Giroscopio (gyro_y) en el chart de giroscopio
    seriesGy->append(t_sec, telemetryData.gyro_y);

    // Actualizamos el eje X para que "corra" con el tiempo
    // Mantenemos una ventana de 10 segundos
    double minX = t_sec - 10.0;
    double maxX = t_sec;
    accAxisX->setRange(minX, maxX);
    gyroAxisX->setRange(minX, maxX);

    // --- AUTO-ESCALA EJE Y (Aceleración) ---
    qreal minAcc = 1000.0, maxAcc = -1000.0;
    // Iteramos sobre los puntos visibles para encontrar min/max
    const QList<QPointF> pointsAx = seriesAx->points();
    if (!pointsAx.isEmpty()) {
        for (const QPointF &p : pointsAx) {
            if (p.x() >= minX) { // Solo consideramos los puntos dentro de la ventana de tiempo
                if (p.y() < minAcc) minAcc = p.y();
                if (p.y() > maxAcc) maxAcc = p.y();
            }
        }
        // Agregamos un margen del 10%
        qreal marginAcc = (maxAcc - minAcc) * 0.1;
        if (marginAcc == 0) marginAcc = 1.0; // Evitar rango nulo
        accAxisY->setRange(minAcc - marginAcc, maxAcc + marginAcc);
    }

    // --- AUTO-ESCALA EJE Y (Giroscopio) ---
    qreal minGyro = 10000.0, maxGyro = -10000.0;
    const QList<QPointF> pointsGy = seriesGy->points();
    if (!pointsGy.isEmpty()) {
        for (const QPointF &p : pointsGy) {
            if (p.x() >= minX) {
                if (p.y() < minGyro) minGyro = p.y();
                if (p.y() > maxGyro) maxGyro = p.y();
            }
        }
        qreal marginGyro = (maxGyro - minGyro) * 0.1;
        if (marginGyro == 0) marginGyro = 10.0;
        gyroAxisY->setRange(minGyro - marginGyro, maxGyro + marginGyro);
    }

    seriesRollFilt->append(t_sec, telemetryData.roll_filt);

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
               << telemetryData.accel_roll << ","
               << telemetryData.gyro_y << ","
               << telemetryData.roll_filt << ","
               << telemetryData.error << ","
               << telemetryData.p << ","
               << telemetryData.i << ","
               << telemetryData.d << ","
               << telemetryData.output << ","
               << telemetryData.pwm_cmd << ","
               << telemetryData.pwm_sat << ","
               << telemetryData.sat_flag << ","
               << telemetryData.mR << ","
               << telemetryData.mL << "\n";
    }
}

void MainWindow::toggleRecording()
{
    if (!isRecording) {
        // Iniciar grabación
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString filename = "telemetry_" + timestamp + ".csv";

        // Crear carpeta logs explícitamente en el directorio de la aplicación
        QDir().mkpath("logs");

        csvLogFile.setFileName("logs/" + filename);
        if (csvLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&csvLogFile);
            // Escribir encabezado de acuerdo a la estructura TelemetryData
            stream << "t_ms,dt_us,accel_roll,gyro_y,roll_filt,error,p,i,d,output,pwm_cmd,pwm_sat,sat_flag,mR,mL\n";

            isRecording = true;
            ui->pushButton_RECORD->setText("STOP");
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
            csvLogFile.close();
        }
        isRecording = false;
        ui->pushButton_RECORD->setText("RECORD");
        ui->pushButton_RECORD->setChecked(false);
        ui->textEdit_PROCCES->append("Grabación detenida.");
    }
}
