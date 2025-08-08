#include "mainwindow.h"
#include "ui_mainwindow.h"

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

    ui->comboBox_CMD->addItem("ALIVE", 0xF0);
    ui->comboBox_CMD->addItem("FIRMWARE", 0xF1);
    ui->comboBox_CMD->addItem("GETADCVALUES", 0xA5);
    ui->comboBox_CMD->addItem("MPU6050", 0xA6);
    ui->comboBox_CMD->addItem("ANGLE", 0xA7);
    ui->comboBox_CMD->addItem("MOTORES", 0xA1);
    ui->comboBox_CMD->addItem("VELOCIDAD", 0xA4);
    ui->comboBox_CMD->addItem("SENDALLSENSORS", 0xA9);

    estadoProtocolo=START;
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
    seriesAx = new QLineSeries(); seriesAx->setName("Acc X");
    seriesAy = new QLineSeries(); seriesAy->setName("Acc Y");
    seriesAz = new QLineSeries(); seriesAz->setName("Acc Z");

    seriesGx = new QLineSeries(); seriesGx->setName("Gyr X");
    seriesGy = new QLineSeries(); seriesGy->setName("Gyr Y");
    seriesGz = new QLineSeries(); seriesGz->setName("Gyr Z");

    // 4) Añade sólo las series de aceleración al accChart
    accChart->addSeries(seriesAx);
    accChart->addSeries(seriesAy);
    accChart->addSeries(seriesAz);

    //    y las de giroscopio al gyroChart
    gyroChart->addSeries(seriesGx);
    gyroChart->addSeries(seriesGy);
    gyroChart->addSeries(seriesGz);

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
    for (auto *s : {seriesAx, seriesAy, seriesAz}) {
        s->attachAxis(accAxisX);
        s->attachAxis(accAxisY);
    }
    for (auto *s : {seriesGx, seriesGy, seriesGz}) {
        s->attachAxis(gyroAxisX);
        s->attachAxis(gyroAxisY);
    }

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

    // Configuro el positionWidget:
    positionChartView = ui->positionWidget;
    positionChart     = new QChart();
    positionChartView->setChart(positionChart);
    positionChartView->setRenderHint(QPainter::Antialiasing);

    positionChart->setBackgroundBrush(QBrush(Qt::darkGray));
    positionChart->legend()->hide();
    positionChart->setMargins(QMargins(0,0,0,0));

    // Ejes X/Y
    posAxisX = new QValueAxis();
    posAxisX->setRange(-100, 100);
    posAxisX->setTitleText("X");
    posAxisY = new QValueAxis();
    posAxisY->setRange(-100, 100);
    posAxisY->setTitleText("Y");
    positionChart->addAxis(posAxisX, Qt::AlignBottom);
    positionChart->addAxis(posAxisY, Qt::AlignLeft);

    // Serie del punto
    positionSeries = new QScatterSeries();
    positionSeries->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    positionSeries->setMarkerSize(12);
    positionSeries->setColor(Qt::green);
    positionChart->addSeries(positionSeries);
    positionSeries->attachAxis(posAxisX);
    positionSeries->attachAxis(posAxisY);
    posScale = 800.0 / 16384.0; // ±800

    // Border de referencia
    borderSeries = new QLineSeries();
    QVector<QPointF> rect = {{-20,-20},{20,-20},{20,20},{-20,20},{-20,-20}};
    borderSeries->append(rect);
    borderSeries->setColor(Qt::lightGray);
    positionChart->addSeries(borderSeries);
    borderSeries->attachAxis(posAxisX);
    borderSeries->attachAxis(posAxisY);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSerialPorts(){
    const SettingsDialog::Settings p = settingPorts->settings();
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

    QString str="";

    for(int i=0; i<=count; i++){
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
    int Port;
    bool ok;

    if(UdpSocket1->isOpen()){
        UdpSocket1->close();
        ui->pushButton_OPENUDP->setText("OPEN UDP");
        return;
    }

    Port=ui->lineEdit_LOCALPORT->text().toInt(&ok,10);
    if(!ok || Port<=0 || Port>65535){
        QMessageBox::information(this, tr("SERVER PORT"),tr("ERRRO. Number PORT."));
        return;
    }

    try{
        UdpSocket1->abort();
        UdpSocket1->bind(Port);
        UdpSocket1->open(QUdpSocket::ReadWrite);
    }catch(...){
        QMessageBox::information(this, tr("SERVER PORT"),tr("Can't OPEN Port."));
        return;
    }
    ui->pushButton_OPENUDP->setText("CLOSE UDP");
    ui->pushButton_UDP->setEnabled(true);
    if(UdpSocket1->isOpen()){
        if(clientAddress.isNull())
            clientAddress.setAddress(ui->lineEdit_IP_REMOTA->text());
        if(puertoremoto==0)
            puertoremoto=ui->lineEdit_DEVICEPORT->text().toInt();
        UdpSocket1->writeDatagram("r", 1, clientAddress, puertoremoto);
    }
}



void MainWindow::OnUdpRxData()
{
    qint64          count=0;
    unsigned char   *incomingBuffer;

    while(UdpSocket1->hasPendingDatagrams()){
        count = UdpSocket1->pendingDatagramSize();
        incomingBuffer = new unsigned char[count];
        UdpSocket1->readDatagram( reinterpret_cast<char *>(incomingBuffer), count, &RemoteAddress, &RemotePort);
    }
    if (count<=0)
        return;

    QString str="";
    for(int i=0; i<=count; i++){
        if(isalnum(incomingBuffer[i]))
            str = str + QString("%1").arg(char(incomingBuffer[i]));
        else
            str = str +"{" + QString("%1").arg(incomingBuffer[i],2,16,QChar('0')) + "}";
    }
    ui->textEdit_RAW->append("MBED-->UDP-->PC (" + str + ")");
    QString adress=RemoteAddress.toString();
    ui->textEdit_RAW->append(" adr " + adress);
    ui->lineEdit_IP_REMOTA->setText(RemoteAddress.toString().right((RemoteAddress.toString().length())-7));
    ui->lineEdit_DEVICEPORT->setText(QString().number(RemotePort,10));

    // 4) Procesamos byte a byte según tu protocolo
    for (int i = 0; i < count; ++i) {
        unsigned char b = incomingBuffer[i];
        switch (estadoProtocoloUdp) {
        case START:
            if (b == 'U') {
                estadoProtocoloUdp = HEADER_1;
                rxDataUdp.cheksum = 0;
            }
            break;
        case HEADER_1:
            if (b == 'N')
                estadoProtocoloUdp = HEADER_2;
            else
                estadoProtocoloUdp = START;
            break;
        case HEADER_2:
            if (b == 'E')
                estadoProtocoloUdp = HEADER_3;
            else
                estadoProtocoloUdp = START;
            break;
        case HEADER_3:
            if (b == 'R')
                estadoProtocoloUdp = NBYTES;
            else
                estadoProtocoloUdp = START;
            break;
        case NBYTES:
            rxDataUdp.nBytes = b;
            estadoProtocoloUdp = TOKEN;
            break;
        case TOKEN:
            if (b == ':') {
                estadoProtocoloUdp = PAYLOAD;
                rxDataUdp.cheksum = 'U' ^ 'N' ^ 'E' ^ 'R' ^ rxDataUdp.nBytes ^ ':';
                rxDataUdp.payLoad[0] = rxDataUdp.nBytes;
                rxDataUdp.index = 1;
            } else {
                estadoProtocoloUdp = START;
            }
            break;
        case PAYLOAD:
            if (rxDataUdp.nBytes > 1) {
                rxDataUdp.payLoad[rxDataUdp.index++] = b;
                rxDataUdp.cheksum ^= b;
            }
            rxDataUdp.nBytes--;
            if (rxDataUdp.nBytes == 0) {
                estadoProtocoloUdp = START;
                if (rxDataUdp.cheksum == b) {
                    decodeData(&rxDataUdp.payLoad[0], UDP);
                } else {
                    ui->textEdit_RAW->append(" CHK DISTINTO!!!!! ");
                }
            }
            break;
        default:
            estadoProtocoloUdp = START;
            break;
        }
    }

    // 5) Liberamos memoria
    delete [] incomingBuffer;
}






/*void MainWindow::OnUdpRxData(){
    qint64          count=0;
    unsigned char   *incomingBuffer;

    while(UdpSocket1->hasPendingDatagrams()){
        count = UdpSocket1->pendingDatagramSize();
        incomingBuffer = new unsigned char[count];
        UdpSocket1->readDatagram( reinterpret_cast<char *>(incomingBuffer), count, &RemoteAddress, &RemotePort);
    }
    if (count<=0)
        return;

    QString str="";
    for(int i=0; i<=count; i++){
        if(isalnum(incomingBuffer[i]))
            str = str + QString("%1").arg(char(incomingBuffer[i]));
        else
            str = str +"{" + QString("%1").arg(incomingBuffer[i],2,16,QChar('0')) + "}";
    }
    ui->textEdit_RAW->append("MBED-->UDP-->PC (" + str + ")");
    QString adress=RemoteAddress.toString();
    ui->textEdit_RAW->append(" adr " + adress);
    ui->lineEdit_IP_REMOTA->setText(RemoteAddress.toString().right((RemoteAddress.toString().length())-7));
    ui->lineEdit_DEVICEPORT->setText(QString().number(RemotePort,10));

    for(int i=0;i<count; i++){
        switch (estadoProtocoloUdp) {
        case START:
            if (incomingBuffer[i]=='U'){
                    estadoProtocoloUdp=HEADER_1;
                    rxDataUdp.cheksum=0;
            }
            break;
        case HEADER_1:
            if (incomingBuffer[i]=='N')
                    estadoProtocoloUdp=HEADER_2;
            else{
                    i--;
                    estadoProtocoloUdp=START;
            }
            break;
        case HEADER_2:
            if (incomingBuffer[i]=='E')
                    estadoProtocoloUdp=HEADER_3;
            else{
                    i--;
                    estadoProtocoloUdp=START;
            }
            break;
        case HEADER_3:
            if (incomingBuffer[i]=='R')
                    estadoProtocoloUdp=NBYTES;
            else{
                    i--;
                    estadoProtocoloUdp=START;
            }
            break;
        case NBYTES:
            rxDataUdp.nBytes=incomingBuffer[i];
            estadoProtocoloUdp=TOKEN;
            break;
        case TOKEN:
            if (incomingBuffer[i]==':'){
                    estadoProtocoloUdp=PAYLOAD;
                    rxDataUdp.cheksum='U'^'N'^'E'^'R'^ rxDataUdp.nBytes^':';
                    rxDataUdp.payLoad[0]=rxDataUdp.nBytes;
                    rxDataUdp.index=1;
            }
            else{
                    i--;
                    estadoProtocoloUdp=START;
            }
            break;
        case PAYLOAD:
            if (rxDataUdp.nBytes>1){
                    rxDataUdp.payLoad[rxDataUdp.index++]=incomingBuffer[i];
                    rxDataUdp.cheksum^=incomingBuffer[i];
            }
            rxDataUdp.nBytes--;
            if(rxDataUdp.nBytes==0){
                    estadoProtocoloUdp=START;
                    if(rxDataUdp.cheksum==incomingBuffer[i]){
                    decodeData(&rxDataUdp.payLoad[0],UDP);
                    }else{
                    ui->textEdit_RAW->append(" CHK DISTINTO!!!!! ");
                    }
            }
            break;

        default:
            estadoProtocoloUdp=START;
            break;
        }
    }
    delete [] incomingBuffer;

}*/

void MainWindow::sendDataUDP(){
    uint8_t cmdId;
    _udat w;
    unsigned char dato[256];
    unsigned char indice=0, chk=0;
    QString str;
    int puerto=0;
    bool ok;

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
            return;
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
    case SETSERVOANGLE: //SERVOANGLE=0xA2,
        dato[indice++] =SETSERVOANGLE;
        w.i32 = QInputDialog::getInt(this, "SERVO", "Angulo:", 0, -90, 90, 1, &ok);
        if(!ok)
            return;
        dato[indice++] = w.i8[0];
        dato[NBYTES]= 0x03;
        break;
    case GETALIVE:
    case GETANGLE: //GETANGLE = 0XA7
    case GETADCVALUES: //GETADCVALUES=0xA5
    case GETMPU6050VALUES:  //GETMPU6050VALUES=0xA6
    case SENDALLSENSORS: //SENDALLSENSORS=0xA9
    case GETDISTANCE:   //GETDISTANCE=0xA3
    case GETSPEED:  //GETSPEED=0xA4
    case GETSWITCHES:   //GETSWITCHES=0xA5
    case GETFIRMWARE:   //GETFIRMWARE=0xF1
    case GETANALOGSENSORS:  //ANALOGSENSORS=0xA0
    case SETLEDS:
        dato[indice++]=cmdId;
        dato[NBYTES]=0x02;
        break;
    default:
        ;

    }

    puerto=ui->lineEdit_DEVICEPORT->text().toInt();
    puertoremoto=puerto;
    for(int a=0 ;a<indice;a++)
        chk^=dato[a];
    dato[indice]=chk;
    if(clientAddress.isNull())
        clientAddress.setAddress(ui->lineEdit_IP_REMOTA->text());
    if(puertoremoto==0)
        puertoremoto=puerto;
    if(UdpSocket1->isOpen()){
        UdpSocket1->writeDatagram(reinterpret_cast<const char *>(dato), (dato[4]+7), clientAddress, puertoremoto);
    }

    for(int i=0; i<=indice; i++){
        if(isalnum(dato[i]))
            str = str + QString("%1").arg(char(dato[i]));
        else
            str = str +"{" + QString("%1").arg(dato[i],2,16,QChar('0')) + "}";
    }
    str=str + clientAddress.toString() + "  " +  QString().number(puertoremoto,10);
    ui->textEdit_RAW->append("PC--UDP-->MBED ( " + str + " )");
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
    case BOXCATEGORY:   // BOXCATEGORY=0xB3
    case GETALLBOXES:
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
    QString str, strOut;
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
        /*case GETANALOGSENSORS://     ANALOGSENSORS=0xA0,
        w.ui8[0] = datosRx[2];
        w.ui8[1] = datosRx[3];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        strOut = "LEFT IR: " + str;
        ui->textEdit_PROCCES->append(strOut);
        ui->label_LIR->setText(str);
        w.ui8[0] = datosRx[4];
        w.ui8[1] = datosRx[5];
        str = QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        strOut = "CENTER IR: " + str;
        ui->textEdit_PROCCES->append(strOut);
        ui->label_CIR->setText(str);
        w.ui8[0] = datosRx[6];
        w.ui8[1] = datosRx[7];
        str =QString("%1").arg(w.ui16[0], 5, 10, QChar('0'));
        strOut = "RIGHT IR: " + str;
        ui->label_RIR->setText(str);
        ui->textEdit_PROCCES->append(strOut);
        break;
    case SETSERVOANGLE://     SERVOANGLE=0xA2,
        if(datosRx[2]==0x0D)
            str= "Servo moviendose. Esperando posición Final!!!";
                else{
                if(datosRx[2]==0x0A)
                    str= "Servo en posición Final!!!";
            }
        ui->textEdit_PROCCES->append(str);
        break;
    case GETDISTANCE://     GETDISTANCE=0xA3,
        w.ui8[0] = datosRx[2];
        w.ui8[1] = datosRx[3];
        w.ui8[2] = datosRx[4];
        w.ui8[3] = datosRx[5];
        str = QString().number(w.ui32/58);
        ui->label_DISTANCE->setText(str+ "cm");
        ui->textEdit_PROCCES->append("DISTANCIA: "+QString().number(w.ui32/58)+ "cm");
        break;
    case GETSPEED://     GETSPEED=0xA4,
        str = "VM1: ";
        w.ui8[0] = datosRx[2];
        w.ui8[1] = datosRx[3];
        w.ui8[2] = datosRx[4];
        w.ui8[3] = datosRx[5];
        strOut = QString("%1").arg(w.i32, 4, 10, QChar('0'));
        ui->label_LENC->setText(strOut);
        str = str + QString("%1").arg(w.i32, 4, 10, QChar('0')) + " - VM2: ";
        w.ui8[0] = datosRx[6];
        w.ui8[1] = datosRx[7];
        w.ui8[2] = datosRx[8];
        w.ui8[3] = datosRx[9];
        strOut = QString("%1").arg(w.i32, 4, 10, QChar('0'));
        ui->label_RENC->setText(strOut);
        str = str + QString("%1").arg(w.i32, 4, 10, QChar('0'));
        ui->textEdit_PROCCES->append(str);
        break;
    case GETSWITCHES: //GETSWITCHES=0xA5
        str = "SW3: ";
        if(datosRx[2] & 0x08)
            str = str + "HIGH";
        else
            str = str + "LOW";
        str = str + " - SW2: ";
        if(datosRx[2] & 0x04)
            str = str + "HIGH";
        else
            str = str + "LOW";
        str = str + " - SW1: ";
        if(datosRx[2] & 0x02)
            str = str + "HIGH";
        else
            str = str + "LOW";
        str = str + " - SW0: ";
        if(datosRx[2] & 0x01)
            str = str + "HIGH";
        else
            str = str + "LOW";
        ui->textEdit_PROCCES->append(str);
        break;*/

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

    case BOXCATEGORY:
        str = "Tipo de Caja: ";
        switch (datosRx[2]) {
        case CATEGORY_A:
            str += "A";
            break;
        case CATEGORY_B:
            str += "B";
            break;
        case CATEGORY_C:
            str += "C";
            break;
        default:
            str += "Desconocido";
            break;
        }
        ui->textEdit_PROCCES->append(str);
        //ui->label_tipoCaja->setText(str);  // opcional, si tenés un label para mostrarlo
        break;

    case SERVOAPATEO: // SERVOAPATEO = 0xB4,
        str = "Servo A pateó";
              ui->textEdit_PROCCES->append(str);
        break;

    case SERVOBPATEO: // SERVOBPATEO = 0xB5,
        str = "Servo B pateó";
              ui->textEdit_PROCCES->append(str);
        break;

    case SERVOCPATEO: // SERVOCPATEO = 0xB6,
        str = "Servo C pateó";
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

        t = elapsedSec;
        seriesAx->append(t, w.i16[0]);
        seriesAy->append(t, w.i16[1]);
        seriesAz->append(t, w.i16[2]);
        seriesGx->append(t, w.i16[3]);
        seriesGy->append(t, w.i16[4]);
        seriesGz->append(t, w.i16[5]);

        t = elapsedSec;
        elapsedSec += SAMPLE_INTERVAL_S;
        accAxisX->setRange(elapsedSec - 10, elapsedSec);

        updatePosition();
        break;
    case GETANGLE: {
        // ROLL
        w.ui8[0] = datosRx[2];
        w.ui8[1] = datosRx[3];
        qint16 roll = w.i16[0];  // firmado
        QString strRoll = QString::number(roll) + " \u00B0";
        ui->label_inclination_Xvalue->setText(strRoll);

        // PITCH
        w.ui8[0] = datosRx[4];
        w.ui8[1] = datosRx[5];
        qint16 pitch = w.i16[0];
        QString strPitch = QString::number(pitch) + " \u00B0";
        ui->label_inclination_Yvalue->setText(strPitch);

        ui->textEdit_PROCCES->append("Valores de inclinacion obtenidos correctamente!");
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
