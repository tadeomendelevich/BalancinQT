#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QtSerialPort/QSerialPort>
#include <QtNetwork/QUdpSocket>
#include <QLabel>
#include <QInputDialog>
#include <QTimer>
#include "settingsdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_ALIVE_clicked();

    void openSerialPorts();

    void closeSerialPorts();

    void dataRecived();

    void decodeData(uint8_t *datosRx, uint8_t source);

    void sendDataSerial();

    void timeOut();

    void on_pushButton_OPENUDP_clicked();

    void sendDataUDP();

    void OnUdpRxData();

    void on_pushButton_clicked();

    void on_pushButton_released();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    SettingsDialog *settingPorts;
    QLabel *estadoSerial;
    QTimer  *timer1;

    QUdpSocket *UdpSocket1;
    QHostAddress RemoteAddress;
    quint16 RemotePort;
    QHostAddress clientAddress;
    int puertoremoto;

    typedef enum{
        START,
        HEADER_1,
        HEADER_2,
        HEADER_3,
        NBYTES,
        TOKEN,
        PAYLOAD
    }_eProtocolo;

    _eProtocolo estadoProtocolo,estadoProtocoloUdp;

    typedef enum{
        UDP=0,
        SERIE=1,
        ACK=0x0D,
        GETALIVE=0xF0,
        GETFIRMWARE=0xF1,
        UNKNOWCMD=0xFF,
        SETLEDS=0x10,
        GETSWITCHES=0x12,
        GETANALOGSENSORS=0xA0,
        SETMOTORTEST=0xA1,
        SETSERVOANGLE=0xA2,
        SERVOMOVESTOP=0x0A,
        GETDISTANCE=0xA3,
        GETSPEED=0xA4,
        SETSERVOLIMITS=0xA5,
        SETBLACKCOLORDETECTED=0xA6,
        SETWHITECOLORDETECTED=0xA7,
        SENDALLSENSORS=0xA9,
        STOPALLSENSORS=0xAA,
        BOXCATEGORY = 0xB3,
        SERVOAPATEO = 0xB4,
        SERVOBPATEO = 0xB5,
        SERVOCPATEO = 0xB6,
        GETALLBOXES = 0xB7,
        CAJASPATEADAS = 0xB8,
        OTHERS
    }_eCmd;


    enum BoxCategory {
        CATEGORY_NONE = 0,
        CATEGORY_A = 1,
        CATEGORY_B = 2,
        CATEGORY_C = 3
    };

    typedef struct{
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[256];
        uint8_t nBytes;
        uint8_t index;
    }_sDatos ;

    _sDatos rxData, rxDataUdp;

    typedef union {
        double  d32;
        float f32;
        int i32;
        unsigned int ui32;
        unsigned short ui16[2];
        short i16[2];
        uint8_t ui8[4];
        char chr[4];
        unsigned char uchr[4];
        int8_t  i8[4];
    }_udat;

    _udat myWord;

    int contadorAlive=0;

};
#endif // MAINWINDOW_H
