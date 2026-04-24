#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QtSerialPort/QSerialPort>
#include <QtNetwork/QUdpSocket>
#include <QLabel>
#include <QInputDialog>
#include <QTimer>
#include <QElapsedTimer>
#include <QFile>
#include "settingsdialog.h"
#include "robotviewer3d.h"

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QScatterSeries>

#define SAMPLE_INTERVAL_MS 30   // Intervalo de muestreo en milisegundos de todos los sensores
#define SAMPLE_INTERVAL_S  (static_cast<double>(SAMPLE_INTERVAL_MS) / 1000.0)   // Deriva el intervalo en segundos (double)

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
    void on_pushButton_BALANCE_clicked();

    void on_pushButton_FOLLOW_LINE_clicked();

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

    void updatePlot();

    void updatePosition();

    void on_pushButton_SetKP_clicked();

    void on_pushButton_SetKD_clicked();

    void on_pushButton_SetKI_clicked();

    void on_pushButton_SetKP_LINE_clicked();

    void on_pushButton_SetKD_LINE_clicked();

    void on_pushButton_SetKI_LINE_clicked();

    void sendManualCommand();

    void on_pushButton_SETPOINT_clicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

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
    int puertoremoto = 0;

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
        SETMOTORSPEED=0xA1,
        SETSERVOANGLE=0xA2,
        SERVOMOVESTOP=0x0A,
        GETDISTANCE=0xA3,
        GETSPEED=0xA4,
        GETADCVALUES=0xA5,
        GETMPU6050VALUES=0xA6,
        GETANGLE = 0XA7,
        SENDALLSENSORS=0xA9,
        STOPALLSENSORS=0xAA,
        MODIFYKP = 0xB1,
        MODIFYKD = 0xB2,
        MODIFYKI = 0xB3,
        BALANCE = 0xB4,
        RESETMASSCENTER = 0xB7,
        ACTIVATE_CSV_LOG = 0xB9,
        ACTIVATE_WIFI_LOG = 0xBA,
        WIFI_LOG_DATA = 0xBB,
        MODIFY_BETA_G = 0xBC,
        MODIFY_BETA_A = 0xBD,
        CHANGE_DISPLAY = 0xBE,
        MODIFY_KV_BRAKE = 0xBF,
        MODIFY_KP_LINE = 0xC0,
        MODIFY_KD_LINE = 0xC1,
        MODIFY_KI_LINE = 0xC2,
        MODIFY_LINE_THRES = 0xC3,
        MODIFY_LINE_SPEED = 0xC4,
        ACTIVATE_LINE_FOLLOWING = 0xC5,
        ACTIVATE_POS_MAINTENANCE = 0xC6,
        ACTIVATE_MANUAL_CONTROL = 0xC7,
        MODIFY_SETPOINT = 0xC8,
        MOVE_FORWARD = 0xD0,
        MOVE_BACKWARD = 0xD1,
        MOVE_LEFT = 0xD2,
        MOVE_RIGHT = 0xD3,
        MOVE_STOP = 0xD4,
        OTHERS
    }_eCmd;


    enum BoxCategory {
        CATEGORY_NONE = 0,
        CATEGORY_A = 1,
        CATEGORY_B = 2,
        CATEGORY_C = 3
    };

    #pragma pack(push, 1)
    struct WifiLogData_t {
        uint32_t t_ms;        // Tiempo en ms desde el inicio
        float    roll_filt;   // Ángulo de inclinación filtrado (grados)
        float    output;      // Salida del PID (antes de ganancia motor)
        float    p_term;      // Término Proporcional
        float    i_term;      // Término Integral
        float    d_term;      // Término Derivativo
        int16_t  mR;          // Velocidad Motor Derecho (-100 a 100)
        int16_t  mL;          // Velocidad Motor Izquierdo (-100 a 100)
        uint32_t dt_ctrl_us;  // Tiempo de control loop delta (microsegundos)
        float    dyn_sp;      // Setpoint dinámico calculado (grados)

        // Telemetría del Seguidor de Línea
        float    line_error;          // Error posicional de la línea
        float    p_line;              // Término Proporcional de la línea
        float    i_line;              // Término Integral de la línea
        float    d_line;              // Término Derivativo de la línea
        float    steering_adjustment; // Salida del PID de la línea (ajuste de giro)
        uint16_t adc1;                // Valor puro ADC 1 (Sensor derecha extremo)
        uint16_t adc2;                // Valor puro ADC 2 (Sensor derecha centro)
        uint16_t adc3;                // Valor puro ADC 3 (Sensor izquierda centro)
        uint16_t adc4;                // Valor puro ADC 4 (Sensor izquierda extremo)
    };
    #pragma pack(pop)

    typedef struct{
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[256];
        uint8_t nBytes;
        uint8_t index;
    }_sDatos ;

    _sDatos rxData, rxDataUdp;

    // Estructura para almacenar los datos de telemetría CSV
    struct TelemetryData {
        uint32_t t_ms;       // Tiempo en ms
        uint32_t dt_us;      // Tiempo entre ciclos (µs)
        float accel_roll;    // Ángulo acelerómetro (grados)
        float gyro_y;        // Velocidad angular (grados/s)
        float roll_filt;     // Ángulo filtrado (grados)
        float dyn_sp;        // Dynamic setpoint de equilibrio
        float error;         // Error actual
        float p;             // Término P
        float i;             // Término I
        float d;             // Término D
        float output;        // Salida PID
        float pwm_cmd;       // PWM calculado
        float pwm_sat;       // PWM saturado
        uint8_t sat_flag;    // Bandera saturación
        int16_t mR;          // Velocidad Motor Derecho
        int16_t mL;          // Velocidad Motor Izquierdo
        float pitch;         // Pitch angle (milli-degrees -> degrees)
        float ax;            // Accel X (m/s^2 * 100 -> m/s^2)
        float ay;            // Accel Y
        float az;            // Accel Z
        float gx;            // Gyro X (deg/s * 100 -> deg/s)
        float gy;            // Gyro Y
        float gz;            // Gyro Z
        uint32_t dt_ctrl_us; // Control Loop DT (µs)
        float gyro_f;        // Low-Pass Filtered Gyro Y (grados/s)
        float accel_roll_f;  // Low-Pass Filtered Accel Roll (grados)
    };

    TelemetryData telemetryData;
    QByteArray csvBuffer; // Buffer para acumular datos CSV incompletos

    void processCsvLine(const QByteArray &line);

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

    QTimer *mpuPollTimer;
    bool    mpuStarted;

    // Tu puntero generado por Designer
    QChartView    *chartView;

    // Accelerometer
    QChartView   *accChartView;
    QChart       *accChart;
    QLineSeries  *seriesAx, *seriesAy, *seriesAz;
    QLineSeries  *seriesAccelRollF; // Nuevo: Roll Filtrado (Low Pass)
    QValueAxis   *accAxisX, *accAxisY;

    // Gyroscope
    QChartView   *gyroChartView;
    QChart       *gyroChart;
    QLineSeries  *seriesGx, *seriesGy, *seriesGz;
    QLineSeries  *seriesGyroF;      // Nuevo: Gyro Y Filtrado
    QValueAxis   *gyroAxisX, *gyroAxisY;

    // Para el gráfico de barras de ADC
    QChartView    *adcChartView;
    QChart        *adcChart;
    QBarSeries    *adcSeries;
    QBarSet       *adcBarSet;
    QBarCategoryAxis *adcAxisX;
    QValueAxis       *adcAxisY;

    // Para llevar el tiempo en el eje X
    qreal          elapsedSec;

    // Grafica posicion MPU6050
    QChartView   *positionChartView;
    QChart       *positionChart;
    QScatterSeries *positionSeries;
    QLineSeries    *borderSeries;
    QValueAxis    *posAxisX;
    QValueAxis    *posAxisY;
    QLineSeries *seriesDynSp;

    // Raw MPU readings, actualizados en decodeData()
    qint16    lastAx = 0;
    qint16    lastAy = 0;
    qint16    lastAz = 0;
    double posScale = 100.0 / 16384.0;  // factor original

    int16_t roll = 0;   // eje x
    int16_t pitch = 0;  // eje y

    // --- NUEVOS GRÁFICOS PARA TABS ---

    // PID Chart
    QChartView *pidChartView;
    QChart *pidChart;
    QLineSeries *seriesP, *seriesI, *seriesD, *seriesOutput, *seriesError;
    QValueAxis *pidAxisX, *pidAxisY;

    QLineSeries *seriesRollFilt; // Para el chart IMU
    QLineSeries *seriesPitch;    // Para el chart IMU (Pitch)

    // Raw Sensors Tab (Nuevo)
    QChartView *rawSensorsAccChartView;
    QChart *rawSensorsAccChart;
    QLineSeries *seriesRawAx, *seriesRawAy, *seriesRawAz;
    QValueAxis *rawSensorsAccAxisX, *rawSensorsAccAxisY;

    QChartView *rawSensorsGyroChartView;
    QChart *rawSensorsGyroChart;
    QLineSeries *seriesRawGx, *seriesRawGy, *seriesRawGz;
    QValueAxis *rawSensorsGyroAxisX, *rawSensorsGyroAxisY;

    // Motors Chart
    QChartView *motorsChartView;
    QChart *motorsChart;
    QLineSeries *seriesPwmCmd, *seriesPwmSat, *seriesMR, *seriesML;
    QValueAxis *motorsAxisX, *motorsAxisY;

    // System Chart
    QChartView *systemChartView;
    QChart *systemChart;
    QLineSeries *seriesDt, *seriesSatFlag;
    QLineSeries *seriesDtCtrl; // Nuevo: Control Loop DT
    QValueAxis *systemAxisX, *systemAxisY;

    // Line Follower Tab (Nuevo)
    QChartView *lineFollowerPidChartView;
    QChart *lineFollowerPidChart;
    QLineSeries *seriesLineError, *seriesLineP, *seriesLineI, *seriesLineD, *seriesLineSteering;
    QValueAxis *lineFollowerPidAxisX, *lineFollowerPidAxisY;

    QChartView *lineFollowerAdcChartView;
    QChart *lineFollowerAdcChart;
    QLineSeries *seriesLineAdc1, *seriesLineAdc2, *seriesLineAdc3, *seriesLineAdc4;
    QValueAxis *lineFollowerAdcAxisX, *lineFollowerAdcAxisY;

    // Vista 3D del robot
    RobotViewer3D *robotViewer3D;

    // GRABACIÓN DE CSV
    QFile csvLogFile;
    bool isRecording = false;
    QString defaultSaveDirectory;

    // Estado de botones
    bool isBalanceActive = false;
    bool isFollowLineActive = false;

    // Control Manual
    QTimer *manualControlTimer;
    uint8_t currentManualCommand = 0;

    // UDP Watchdog
    QTimer *udpWatchdogTimer;
    QElapsedTimer udpLastRxTime;
    bool udpEverReceivedData;
    void checkUdpInactivity();
    void clearUdpScreens();

    void toggleRecording();
    void selectSaveDirectory();
};
#endif // MAINWINDOW_H
