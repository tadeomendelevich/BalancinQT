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
    ui->comboBox_CMD->addItem("SENSORES", 0xA0);
    ui->comboBox_CMD->addItem("MOTORES", 0xA1);
    ui->comboBox_CMD->addItem("SERVO", 0xA2);
    ui->comboBox_CMD->addItem("DISTANCIA", 0xA3);
    ui->comboBox_CMD->addItem("VELOCIDAD", 0xA4);
    ui->comboBox_CMD->addItem("SWITCHS", 0x12);
    ui->comboBox_CMD->addItem("LEDS", 0x10);
    ui->comboBox_CMD->addItem("SENDALLSENSORS", 0xA9);
    ui->comboBox_CMD->addItem("STOPALLSENSORS", 0xAA);
    ui->comboBox_CMD->addItem("BOXCATEGORY", 0xB3);
    ui->comboBox_CMD->addItem("GETALLBOXES", 0xB7);


    estadoProtocolo=START;
    rxData.timeOut=0;
    ui->pushButton_UDP->setEnabled(false);
    ui->pushButton_ALIVE->setEnabled(false);
    ui->pushButton_SEND->setEnabled(false);
    timer1->start(100);
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

void MainWindow::decodeData(uint8_t *datosRx, uint8_t source){
    int32_t length = sizeof(*datosRx)/sizeof(datosRx[0]);
    QString str, strOut;
    _udat w;
    uint8_t cantidad, contadorCajas;
    for(int i = 1; i<length; i++){
        if(isalnum(datosRx[i]))
            str = str + QString("%1").arg(char(datosRx[i]));
        else
            str = str +QString("%1").arg(datosRx[i],2,16,QChar('0'));
    }
    ui->textEdit_RAW->append("*(MBED-S->PC)->decodeData (" + str + ")");

    switch (datosRx[1]) {
    case GETANALOGSENSORS://     ANALOGSENSORS=0xA0,
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
    case SETMOTORTEST://     MOTORTEST=0xA1,
        if(datosRx[2]==0x0D)
            str= "Test Motores ACK";
        ui->textEdit_PROCCES->append(str);
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
        break;

    case GETALIVE://     GETALIVE=0xF0,
        if(datosRx[2]==ACK){
            contadorAlive++;
            if(source)
                    str="ALIVE BLUEPILL VIA *SERIE* RECIBIDO!!!";
            else{
                    contadorAlive++;
                    str="ALIVE BLUEPILL VIA *UDP* RECIBIDO N°: " + QString().number(contadorAlive,10);
            }
        }else{
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

    case CAJASPATEADAS: // CAJASPATEADAS = 0xB8
        ui->textEdit_2->clear();
        contadorCajas = datosRx[2];
        str = QString("<span style='font-size:18pt;'><b>CAJAS PATEADAS: %1</b></span>").arg(contadorCajas);
        ui->textEdit_2->setHtml(str);
        break;

    case GETALLBOXES: // GETALLBOXES
        ui->textEdit->clear();
        ui->textEdit->append("                              Box buffer");

        str = "Tipos de cajas en buffer: [";

        cantidad = datosRx[0] - 2;  // Descontamos ID y checksum
        for (int i = 2; i < 2 + cantidad; i++) {
            switch (datosRx[i]) {
            case CATEGORY_A:     str += "A "; break;
            case CATEGORY_B:     str += "B "; break;
            case CATEGORY_C:     str += "C "; break;
            case CATEGORY_NONE:  str += "Desconocido "; break;
            default:             str += "? "; break;
            }
        }

        str += " ]";
        ui->textEdit->append(str);

        str = "GETALLBOXES Payload crudo: ";
        for (int i = 1; i <= datosRx[0]; i++) {
            str += QString("[%1]").arg(datosRx[i], 2, 16, QChar('0'));
        }
        ui->textEdit_RAW->append(str);
        break;



    default:
        str = str + "Comando DESCONOCIDO!!!!";
        ui->textEdit_PROCCES->append(str);
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
    case SETMOTORTEST://MOTORTEST=0xA1,
        dato[indice++] =SETMOTORTEST;
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
        ui->lineEdit_IP_REMOTA->clear();
        ui->lineEdit_DEVICEPORT->clear();
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

void MainWindow::OnUdpRxData(){
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

}

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
    case SETMOTORTEST://MOTORTEST=0xA1,
        dato[indice++] =SETMOTORTEST;
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
    case SETSERVOANGLE://SERVOANGLE=0xA2,
        dato[indice++] =SETSERVOANGLE;
        w.i32 = QInputDialog::getInt(this, "SERVO", "Angulo:", 0, -90, 90, 1, &ok);
        if(!ok)
            return;
        dato[indice++] = w.i8[0];
        dato[NBYTES]= 0x03;
        break;
    case GETALIVE:
    case GETDISTANCE://GETDISTANCE=0xA3,
    case GETSPEED://GETSPEED=0xA4,
    case GETSWITCHES://GETSWITCHES=0xA5
    case GETFIRMWARE:// GETFIRMWARE=0xF1
    case GETANALOGSENSORS://ANALOGSENSORS=0xA0,
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



void MainWindow::on_pushButton_clicked()    // BOTON DE "CLEAR DISPLAYS"
{
    ui->textEdit_PROCCES->clear();
    ui->textEdit_RAW->clear();
    ui->textEdit->clear();
    ui->textEdit_2->clear();
    ui->textEdit_RAW->append("           Dato sin procesar");
    ui->textEdit_PROCCES->append("       Dato procesado");
    ui->textEdit->append("                              Box buffer");

    ui->textEdit->append("                              CAJAS PATEADAS");
}
