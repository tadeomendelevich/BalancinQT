/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.5.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCharts/QChartView>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionScanPorts;
    QAction *actionConnect_Device;
    QAction *actionDisconnect_Device;
    QAction *actionQuit;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QSplitter *splitter_main;
    QTabWidget *tabWidget_Graficas;
    QWidget *tab_IMU;
    QVBoxLayout *verticalLayout_TabIMU;
    QHBoxLayout *horizontalLayout_Angles;
    QLabel *label_inclination_X;
    QLabel *label_inclination_Xvalue;
    QLabel *label_inclination_Y;
    QLabel *label_inclination_Yvalue;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_LogDir;
    QLabel *label_LogDir;
    QLineEdit *lineEdit_LogDir;
    QPushButton *btnSelectLogDir;
    QLabel *label_10;
    QChartView *accelerationWidget;
    QLabel *label_11;
    QChartView *gyroscopeWidget;
    QWidget *tab_PID;
    QVBoxLayout *verticalLayout_TabPID;
    QChartView *pidWidget;
    QWidget *tab_Motors;
    QVBoxLayout *verticalLayout_TabMotors;
    QChartView *motorsWidget;
    QWidget *tab_System;
    QVBoxLayout *verticalLayout_TabSystem;
    QChartView *systemWidget;
    QWidget *widget_controls;
    QSplitter *splitter_texts;
    QTextEdit *textEdit_RAW;
    QTextEdit *textEdit_PROCCES;
    QVBoxLayout *verticalLayout_6;
    QVBoxLayout *verticalLayout_5;
    QComboBox *comboBox_CMD;
    QHBoxLayout *horizontalLayout;
    QPushButton *pushButton_SEND;
    QPushButton *pushButton_BALANCE;
    QPushButton *pushButton_FOLLOW_LINE;
    QPushButton *pushButton_UDP;
    QPushButton *pushButton_RECORD;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_9;
    QLineEdit *lineEdit_LOCALPORT;
    QPushButton *pushButton_OPENUDP;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_7;
    QLineEdit *lineEdit_IP_REMOTA;
    QHBoxLayout *horizontalLayout_9;
    QLabel *label_8;
    QLineEdit *lineEdit_DEVICEPORT;
    QVBoxLayout *verticalLayout_13;
    QVBoxLayout *verticalLayout_16;
    QLabel *label;
    QHBoxLayout *horizontalLayout_16;
    QLabel *label_13;
    QLabel *label_14;
    QLabel *label_5;
    QVBoxLayout *verticalLayout_14;
    QHBoxLayout *horizontalLayout_15;
    QLabel *label_KP;
    QLabel *label_KD;
    QLabel *label_KI;
    QHBoxLayout *horizontalLayout_PID_Inputs;
    QHBoxLayout *horizontalLayout_KP;
    QLineEdit *lineEdit_KP;
    QPushButton *pushButton_SetKP;
    QHBoxLayout *horizontalLayout_KD;
    QLineEdit *lineEdit_KD;
    QPushButton *pushButton_SetKD;
    QHBoxLayout *horizontalLayout_KI;
    QLineEdit *lineEdit_KI;
    QPushButton *pushButton_SetKI;
    QVBoxLayout *verticalLayout_PID_LINE;
    QLabel *label_PID_LINE_title;
    QHBoxLayout *horizontalLayout_PID_LINE_headers;
    QLabel *label_PID_LINE_KP_hdr;
    QLabel *label_PID_LINE_KD_hdr;
    QLabel *label_PID_LINE_KI_hdr;
    QHBoxLayout *horizontalLayout_PID_LINE_values;
    QLabel *label_KP_LINE;
    QLabel *label_KD_LINE;
    QLabel *label_KI_LINE;
    QHBoxLayout *horizontalLayout_PID_LINE_inputs;
    QHBoxLayout *horizontalLayout_KP_LINE;
    QLineEdit *lineEdit_KP_LINE;
    QPushButton *pushButton_SetKP_LINE;
    QHBoxLayout *horizontalLayout_KD_LINE;
    QLineEdit *lineEdit_KD_LINE;
    QPushButton *pushButton_SetKD_LINE;
    QHBoxLayout *horizontalLayout_KI_LINE;
    QLineEdit *lineEdit_KI_LINE;
    QPushButton *pushButton_SetKI_LINE;
    QLabel *label_SETPOINT;
    QHBoxLayout *horizontalLayout_5;
    QDoubleSpinBox *spinBox_SETPOINT;
    QPushButton *pushButton_SETPOINT;
    QGridLayout *gridLayout_DirectionButtons;
    QPushButton *btn_UP;
    QPushButton *btn_LEFT;
    QPushButton *btn_RIGHT;
    QPushButton *btn_DOWN;
    QPushButton *pushButton;
    QMenuBar *menubar;
    QMenu *menuDevice;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1317, 928);
        MainWindow->setStyleSheet(QString::fromUtf8("/* ============ BalancinQT Global Theme ============ */\n"
"#centralwidget {\n"
"    background-color: #0d1a08;\n"
"    background-image: url(:/img/logoUNERsolo.png);\n"
"    background-repeat: no-repeat;\n"
"    background-position: center;\n"
"}\n"
"QMenuBar {\n"
"    background-color: #111e08;\n"
"    color: #b8dfa0;\n"
"    border-bottom: 1px solid #3a5f0b;\n"
"    padding: 2px;\n"
"}\n"
"QMenuBar::item {\n"
"    background-color: transparent;\n"
"    padding: 4px 12px;\n"
"    border-radius: 4px;\n"
"}\n"
"QMenuBar::item:selected { background-color: #2e4c07; }\n"
"QMenu {\n"
"    background-color: #182e08;\n"
"    color: #d0ffcc;\n"
"    border: 1px solid #4a6620;\n"
"    padding: 4px;\n"
"}\n"
"QMenu::item {\n"
"    padding: 5px 20px 5px 10px;\n"
"    border-radius: 3px;\n"
"}\n"
"QMenu::item:selected { background-color: #3a5f0b; }\n"
"QMenu::separator {\n"
"    height: 1px;\n"
"    background: #3a5f0b;\n"
"    margin: 3px 8px;\n"
"}\n"
"QPushButton {\n"
"    color: #e8ffe0;\n"
"    background-color: qli"
                        "neargradient(x1:0, y1:0, x2:0, y2:1,\n"
"        stop:0 #4e6e18, stop:1 #2a4808);\n"
"    border: 1px solid #3a5a10;\n"
"    border-radius: 6px;\n"
"    padding: 5px 12px;\n"
"    font-weight: bold;\n"
"    font-size: 11px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
"        stop:0 #6b8e23, stop:1 #3a5f0b);\n"
"    border-color: #7aaa28;\n"
"}\n"
"QPushButton:pressed {\n"
"    background-color: #1e3506;\n"
"    border-color: #8aab30;\n"
"}\n"
"QPushButton:disabled {\n"
"    background-color: #1a2a10;\n"
"    color: #4a5a3a;\n"
"    border-color: #202e12;\n"
"}\n"
"QPushButton:checked {\n"
"    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
"        stop:0 #b01010, stop:1 #6a0808);\n"
"    border: 2px solid #e04040;\n"
"    color: white;\n"
"}\n"
"QPushButton#btn_UP,\n"
"QPushButton#btn_DOWN,\n"
"QPushButton#btn_LEFT,\n"
"QPushButton#btn_RIGHT {\n"
"    font-size: 20px;\n"
"    padding: 8px;\n"
"    border-radius: 10px;\n"
"}\n"
"QLabel {\n"
""
                        "    color: #c8e6a0;\n"
"    background-color: transparent;\n"
"    border: none;\n"
"    padding: 2px 4px;\n"
"}\n"
"QLabel#label_10, QLabel#label_11, QLabel#label {\n"
"    color: #a0d060;\n"
"    background-color: rgba(30, 60, 10, 0.45);\n"
"    border: 1px solid #3a5f0b;\n"
"    border-radius: 5px;\n"
"    padding: 5px;\n"
"}\n"
"QLabel#label_inclination_Xvalue,\n"
"QLabel#label_inclination_Yvalue {\n"
"    color: #44ffcc;\n"
"    background-color: rgba(0, 40, 30, 0.55);\n"
"    border: 1px solid #207060;\n"
"    border-radius: 5px;\n"
"    font-family: Consolas;\n"
"    font-weight: bold;\n"
"    padding: 4px 8px;\n"
"    min-width: 60px;\n"
"}\n"
"QLabel#label_KP, QLabel#label_KD, QLabel#label_KI,\n"
"QLabel#label_KP_LINE, QLabel#label_KD_LINE, QLabel#label_KI_LINE {\n"
"    color: #ffe080;\n"
"    background-color: rgba(40, 30, 0, 0.55);\n"
"    border: 1px solid #706010;\n"
"    border-radius: 5px;\n"
"    font-family: Consolas;\n"
"    padding: 4px 8px;\n"
"}\n"
"QTextEdit {\n"
"    border: 1px solid #"
                        "3a5f0b;\n"
"    border-radius: 8px;\n"
"    background-color: rgba(8, 20, 4, 0.90);\n"
"    color: #b8e8a0;\n"
"    padding: 6px;\n"
"    font-family: Consolas;\n"
"    selection-background-color: #3a5f0b;\n"
"    selection-color: #ffffff;\n"
"}\n"
"QTextEdit#textEdit_RAW {\n"
"    font-size: 12px;\n"
"    color: #78b878;\n"
"    border-color: #2a4510;\n"
"}\n"
"QTextEdit#textEdit_PROCCES {\n"
"    font-size: 13px;\n"
"    color: #c0ffa0;\n"
"    border-color: #5a8020;\n"
"}\n"
"QLineEdit {\n"
"    color: #c8ffb0;\n"
"    background-color: rgba(8, 25, 4, 0.85);\n"
"    border: 1px solid #3a5f0b;\n"
"    border-radius: 6px;\n"
"    padding: 4px 8px;\n"
"    selection-background-color: #3a5f0b;\n"
"    selection-color: white;\n"
"}\n"
"QLineEdit:focus {\n"
"    border-color: #7aaa28;\n"
"    background-color: rgba(12, 35, 5, 0.95);\n"
"}\n"
"QLineEdit:read-only {\n"
"    color: #78a870;\n"
"    background-color: rgba(5, 18, 2, 0.75);\n"
"}\n"
"QDoubleSpinBox, QSpinBox {\n"
"    color: #c8ffb0;\n"
"    background"
                        "-color: rgba(8, 25, 4, 0.85);\n"
"    border: 1px solid #3a5f0b;\n"
"    border-radius: 6px;\n"
"    padding: 4px 8px;\n"
"    font-family: Consolas;\n"
"    font-size: 12px;\n"
"}\n"
"QDoubleSpinBox:focus, QSpinBox:focus {\n"
"    border-color: #7aaa28;\n"
"    background-color: rgba(12, 35, 5, 0.95);\n"
"}\n"
"QDoubleSpinBox::up-button, QSpinBox::up-button,\n"
"QDoubleSpinBox::down-button, QSpinBox::down-button {\n"
"    background-color: #2e4c07;\n"
"    border: 1px solid #3a5f0b;\n"
"    width: 16px;\n"
"}\n"
"QDoubleSpinBox::up-button:hover, QSpinBox::up-button:hover,\n"
"QDoubleSpinBox::down-button:hover, QSpinBox::down-button:hover {\n"
"    background-color: #4a7818;\n"
"}\n"
"QLabel#label_SETPOINT {\n"
"    color: #44ffdd;\n"
"    background-color: rgba(0, 40, 30, 0.55);\n"
"    border: 1px solid #207060;\n"
"    border-radius: 6px;\n"
"    font-family: Consolas;\n"
"    font-weight: bold;\n"
"    font-size: 13px;\n"
"    padding: 5px 10px;\n"
"    qproperty-alignment: AlignCenter;\n"
"}\n"
"QComboBox"
                        " {\n"
"    color: #d0ffcc;\n"
"    background-color: rgba(15, 40, 8, 0.90);\n"
"    border: 1px solid #3a5f0b;\n"
"    border-radius: 6px;\n"
"    padding: 5px 8px;\n"
"}\n"
"QComboBox:hover { border-color: #6b8e23; }\n"
"QComboBox::drop-down {\n"
"    subcontrol-origin: padding;\n"
"    subcontrol-position: top right;\n"
"    width: 22px;\n"
"    border-left: 1px solid #3a5f0b;\n"
"    border-top-right-radius: 6px;\n"
"    border-bottom-right-radius: 6px;\n"
"}\n"
"QComboBox QAbstractItemView {\n"
"    background-color: #182e08;\n"
"    border: 1px solid #4a6620;\n"
"    color: #d0ffcc;\n"
"    selection-background-color: #3a5f0b;\n"
"    outline: none;\n"
"}\n"
"QTabWidget::pane {\n"
"    border: 1px solid #3a5f0b;\n"
"    border-radius: 5px;\n"
"    background-color: rgba(8, 18, 4, 0.70);\n"
"    top: -1px;\n"
"}\n"
"QTabBar::tab {\n"
"    background-color: #162a08;\n"
"    color: #7a9a60;\n"
"    border: 1px solid #263a10;\n"
"    border-bottom: none;\n"
"    border-top-left-radius: 6px;\n"
"    border-top"
                        "-right-radius: 6px;\n"
"    padding: 6px 16px;\n"
"    margin-right: 2px;\n"
"    font-weight: bold;\n"
"    font-size: 11px;\n"
"}\n"
"QTabBar::tab:selected {\n"
"    background-color: rgba(8, 18, 4, 0.70);\n"
"    color: #c0ffaa;\n"
"    border-color: #5a8020;\n"
"    border-bottom: 2px solid #8aab30;\n"
"}\n"
"QTabBar::tab:hover:!selected {\n"
"    background-color: #1e3a0c;\n"
"    color: #a0d080;\n"
"}\n"
"QScrollBar:vertical {\n"
"    background-color: #0c180a;\n"
"    width: 10px;\n"
"    margin: 0;\n"
"}\n"
"QScrollBar::handle:vertical {\n"
"    background-color: #2e5010;\n"
"    border-radius: 5px;\n"
"    min-height: 24px;\n"
"}\n"
"QScrollBar::handle:vertical:hover { background-color: #4a7818; }\n"
"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }\n"
"QScrollBar:horizontal {\n"
"    background-color: #0c180a;\n"
"    height: 10px;\n"
"    margin: 0;\n"
"}\n"
"QScrollBar::handle:horizontal {\n"
"    background-color: #2e5010;\n"
"    border-radius: 5px;\n"
"    min-width: 2"
                        "4px;\n"
"}\n"
"QScrollBar::handle:horizontal:hover { background-color: #4a7818; }\n"
"QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }\n"
"QStatusBar {\n"
"    background-color: #0d1a08;\n"
"    color: #789060;\n"
"    border-top: 1px solid #2a4010;\n"
"    font-size: 11px;\n"
"}\n"
"QStatusBar QLabel {\n"
"    border: none;\n"
"    background: transparent;\n"
"    padding: 2px 6px;\n"
"}"));
        actionScanPorts = new QAction(MainWindow);
        actionScanPorts->setObjectName("actionScanPorts");
        actionConnect_Device = new QAction(MainWindow);
        actionConnect_Device->setObjectName("actionConnect_Device");
        actionDisconnect_Device = new QAction(MainWindow);
        actionDisconnect_Device->setObjectName("actionDisconnect_Device");
        actionQuit = new QAction(MainWindow);
        actionQuit->setObjectName("actionQuit");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName("gridLayout");
        splitter_main = new QSplitter(centralwidget);
        splitter_main->setObjectName("splitter_main");
        splitter_main->setOrientation(Qt::Horizontal);
        splitter_main->setHandleWidth(6);
        tabWidget_Graficas = new QTabWidget(centralwidget);
        tabWidget_Graficas->setObjectName("tabWidget_Graficas");
        tab_IMU = new QWidget();
        tab_IMU->setObjectName("tab_IMU");
        verticalLayout_TabIMU = new QVBoxLayout(tab_IMU);
        verticalLayout_TabIMU->setObjectName("verticalLayout_TabIMU");
        horizontalLayout_Angles = new QHBoxLayout();
        horizontalLayout_Angles->setObjectName("horizontalLayout_Angles");
        label_inclination_X = new QLabel(tab_IMU);
        label_inclination_X->setObjectName("label_inclination_X");
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        label_inclination_X->setFont(font);

        horizontalLayout_Angles->addWidget(label_inclination_X);

        label_inclination_Xvalue = new QLabel(tab_IMU);
        label_inclination_Xvalue->setObjectName("label_inclination_Xvalue");
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Consolas")});
        font1.setPointSize(12);
        font1.setBold(true);
        label_inclination_Xvalue->setFont(font1);

        horizontalLayout_Angles->addWidget(label_inclination_Xvalue);

        label_inclination_Y = new QLabel(tab_IMU);
        label_inclination_Y->setObjectName("label_inclination_Y");
        label_inclination_Y->setFont(font);

        horizontalLayout_Angles->addWidget(label_inclination_Y);

        label_inclination_Yvalue = new QLabel(tab_IMU);
        label_inclination_Yvalue->setObjectName("label_inclination_Yvalue");
        label_inclination_Yvalue->setFont(font1);

        horizontalLayout_Angles->addWidget(label_inclination_Yvalue);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_Angles->addItem(horizontalSpacer);


        verticalLayout_TabIMU->addLayout(horizontalLayout_Angles);

        horizontalLayout_LogDir = new QHBoxLayout();
        horizontalLayout_LogDir->setObjectName("horizontalLayout_LogDir");
        label_LogDir = new QLabel(tab_IMU);
        label_LogDir->setObjectName("label_LogDir");
        QFont font2;
        font2.setBold(true);
        label_LogDir->setFont(font2);

        horizontalLayout_LogDir->addWidget(label_LogDir);

        lineEdit_LogDir = new QLineEdit(tab_IMU);
        lineEdit_LogDir->setObjectName("lineEdit_LogDir");
        lineEdit_LogDir->setReadOnly(true);

        horizontalLayout_LogDir->addWidget(lineEdit_LogDir);

        btnSelectLogDir = new QPushButton(tab_IMU);
        btnSelectLogDir->setObjectName("btnSelectLogDir");
        btnSelectLogDir->setMaximumSize(QSize(30, 16777215));

        horizontalLayout_LogDir->addWidget(btnSelectLogDir);


        verticalLayout_TabIMU->addLayout(horizontalLayout_LogDir);

        label_10 = new QLabel(tab_IMU);
        label_10->setObjectName("label_10");
        label_10->setMaximumSize(QSize(16777215, 30));
        label_10->setFont(font);
        label_10->setAlignment(Qt::AlignCenter);

        verticalLayout_TabIMU->addWidget(label_10);

        accelerationWidget = new QChartView(tab_IMU);
        accelerationWidget->setObjectName("accelerationWidget");
        accelerationWidget->setMinimumSize(QSize(300, 200));

        verticalLayout_TabIMU->addWidget(accelerationWidget);

        label_11 = new QLabel(tab_IMU);
        label_11->setObjectName("label_11");
        label_11->setMaximumSize(QSize(16777215, 30));
        label_11->setFont(font);
        label_11->setAlignment(Qt::AlignCenter);

        verticalLayout_TabIMU->addWidget(label_11);

        gyroscopeWidget = new QChartView(tab_IMU);
        gyroscopeWidget->setObjectName("gyroscopeWidget");
        gyroscopeWidget->setMinimumSize(QSize(300, 200));

        verticalLayout_TabIMU->addWidget(gyroscopeWidget);

        tabWidget_Graficas->addTab(tab_IMU, QString());
        tab_PID = new QWidget();
        tab_PID->setObjectName("tab_PID");
        verticalLayout_TabPID = new QVBoxLayout(tab_PID);
        verticalLayout_TabPID->setObjectName("verticalLayout_TabPID");
        pidWidget = new QChartView(tab_PID);
        pidWidget->setObjectName("pidWidget");

        verticalLayout_TabPID->addWidget(pidWidget);

        tabWidget_Graficas->addTab(tab_PID, QString());
        tab_Motors = new QWidget();
        tab_Motors->setObjectName("tab_Motors");
        verticalLayout_TabMotors = new QVBoxLayout(tab_Motors);
        verticalLayout_TabMotors->setObjectName("verticalLayout_TabMotors");
        motorsWidget = new QChartView(tab_Motors);
        motorsWidget->setObjectName("motorsWidget");

        verticalLayout_TabMotors->addWidget(motorsWidget);

        tabWidget_Graficas->addTab(tab_Motors, QString());
        tab_System = new QWidget();
        tab_System->setObjectName("tab_System");
        verticalLayout_TabSystem = new QVBoxLayout(tab_System);
        verticalLayout_TabSystem->setObjectName("verticalLayout_TabSystem");
        systemWidget = new QChartView(tab_System);
        systemWidget->setObjectName("systemWidget");

        verticalLayout_TabSystem->addWidget(systemWidget);

        tabWidget_Graficas->addTab(tab_System, QString());

        splitter_main->addWidget(tabWidget_Graficas);

        widget_controls = new QWidget(centralwidget);
        widget_controls->setObjectName("widget_controls");

        splitter_texts = new QSplitter(centralwidget);
        splitter_texts->setObjectName("splitter_texts");
        splitter_texts->setOrientation(Qt::Vertical);
        splitter_texts->setMinimumSize(QSize(200, 0));
        splitter_texts->setMaximumSize(QSize(300, 16777215));
        splitter_texts->setHandleWidth(6);

        QFont font3;
        font3.setFamilies({QString::fromUtf8("Consolas")});
        font3.setBold(false);
        font3.setItalic(false);

        textEdit_RAW = new QTextEdit(splitter_texts);
        textEdit_RAW->setObjectName("textEdit_RAW");
        textEdit_RAW->setFont(font3);
        textEdit_RAW->setFrameShape(QFrame::NoFrame);

        splitter_texts->addWidget(textEdit_RAW);

        textEdit_PROCCES = new QTextEdit(splitter_texts);
        textEdit_PROCCES->setObjectName("textEdit_PROCCES");
        textEdit_PROCCES->setFont(font3);

        splitter_texts->addWidget(textEdit_PROCCES);

        splitter_main->addWidget(splitter_texts);

        verticalLayout_6 = new QVBoxLayout(widget_controls);
        verticalLayout_6->setObjectName("verticalLayout_6");
        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setObjectName("verticalLayout_5");
        comboBox_CMD = new QComboBox(centralwidget);
        comboBox_CMD->setObjectName("comboBox_CMD");

        verticalLayout_5->addWidget(comboBox_CMD);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        pushButton_SEND = new QPushButton(centralwidget);
        pushButton_SEND->setObjectName("pushButton_SEND");

        horizontalLayout->addWidget(pushButton_SEND);

        pushButton_BALANCE = new QPushButton(centralwidget);
        pushButton_BALANCE->setObjectName("pushButton_BALANCE");

        horizontalLayout->addWidget(pushButton_BALANCE);

        pushButton_FOLLOW_LINE = new QPushButton(centralwidget);
        pushButton_FOLLOW_LINE->setObjectName("pushButton_FOLLOW_LINE");

        horizontalLayout->addWidget(pushButton_FOLLOW_LINE);

        pushButton_UDP = new QPushButton(centralwidget);
        pushButton_UDP->setObjectName("pushButton_UDP");

        horizontalLayout->addWidget(pushButton_UDP);

        pushButton_RECORD = new QPushButton(centralwidget);
        pushButton_RECORD->setObjectName("pushButton_RECORD");
        pushButton_RECORD->setCheckable(true);

        horizontalLayout->addWidget(pushButton_RECORD);


        verticalLayout_5->addLayout(horizontalLayout);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_9 = new QLabel(centralwidget);
        label_9->setObjectName("label_9");
        label_9->setFont(font2);

        horizontalLayout_2->addWidget(label_9);

        lineEdit_LOCALPORT = new QLineEdit(centralwidget);
        lineEdit_LOCALPORT->setObjectName("lineEdit_LOCALPORT");

        horizontalLayout_2->addWidget(lineEdit_LOCALPORT);

        pushButton_OPENUDP = new QPushButton(centralwidget);
        pushButton_OPENUDP->setObjectName("pushButton_OPENUDP");
        pushButton_OPENUDP->setMinimumSize(QSize(80, 30));

        horizontalLayout_2->addWidget(pushButton_OPENUDP);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName("horizontalLayout_8");
        label_7 = new QLabel(centralwidget);
        label_7->setObjectName("label_7");
        label_7->setFont(font2);

        horizontalLayout_8->addWidget(label_7);

        lineEdit_IP_REMOTA = new QLineEdit(centralwidget);
        lineEdit_IP_REMOTA->setObjectName("lineEdit_IP_REMOTA");

        horizontalLayout_8->addWidget(lineEdit_IP_REMOTA);


        verticalLayout->addLayout(horizontalLayout_8);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setObjectName("horizontalLayout_9");
        label_8 = new QLabel(centralwidget);
        label_8->setObjectName("label_8");
        label_8->setFont(font2);

        horizontalLayout_9->addWidget(label_8);

        lineEdit_DEVICEPORT = new QLineEdit(centralwidget);
        lineEdit_DEVICEPORT->setObjectName("lineEdit_DEVICEPORT");

        horizontalLayout_9->addWidget(lineEdit_DEVICEPORT);


        verticalLayout->addLayout(horizontalLayout_9);


        verticalLayout_5->addLayout(verticalLayout);


        verticalLayout_6->addLayout(verticalLayout_5);

        verticalLayout_13 = new QVBoxLayout();
        verticalLayout_13->setObjectName("verticalLayout_13");
        verticalLayout_16 = new QVBoxLayout();
        verticalLayout_16->setObjectName("verticalLayout_16");
        label = new QLabel(centralwidget);
        label->setObjectName("label");
        label->setMaximumSize(QSize(16777215, 30));
        QFont font4;
        font4.setPointSize(14);
        font4.setBold(true);
        label->setFont(font4);
        label->setAlignment(Qt::AlignCenter);

        verticalLayout_16->addWidget(label);


        verticalLayout_13->addLayout(verticalLayout_16);

        horizontalLayout_16 = new QHBoxLayout();
        horizontalLayout_16->setObjectName("horizontalLayout_16");
        label_13 = new QLabel(centralwidget);
        label_13->setObjectName("label_13");
        label_13->setMaximumSize(QSize(16777215, 30));
        label_13->setFont(font);
        label_13->setAlignment(Qt::AlignCenter);

        horizontalLayout_16->addWidget(label_13);

        label_14 = new QLabel(centralwidget);
        label_14->setObjectName("label_14");
        label_14->setMaximumSize(QSize(16777215, 30));
        label_14->setFont(font);
        label_14->setAlignment(Qt::AlignCenter);

        horizontalLayout_16->addWidget(label_14);

        label_5 = new QLabel(centralwidget);
        label_5->setObjectName("label_5");
        label_5->setMaximumSize(QSize(16777215, 30));
        label_5->setFont(font);
        label_5->setAlignment(Qt::AlignCenter);

        horizontalLayout_16->addWidget(label_5);


        verticalLayout_13->addLayout(horizontalLayout_16);

        verticalLayout_14 = new QVBoxLayout();
        verticalLayout_14->setObjectName("verticalLayout_14");
        horizontalLayout_15 = new QHBoxLayout();
        horizontalLayout_15->setObjectName("horizontalLayout_15");
        label_KP = new QLabel(centralwidget);
        label_KP->setObjectName("label_KP");
        label_KP->setMaximumSize(QSize(16777215, 25));
        QFont font5;
        font5.setFamilies({QString::fromUtf8("Consolas")});
        font5.setPointSize(11);
        label_KP->setFont(font5);
        label_KP->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(label_KP);

        label_KD = new QLabel(centralwidget);
        label_KD->setObjectName("label_KD");
        label_KD->setMaximumSize(QSize(16777215, 25));
        QFont font6;
        font6.setFamilies({QString::fromUtf8("Consolas")});
        font6.setPointSize(12);
        label_KD->setFont(font6);
        label_KD->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(label_KD);

        label_KI = new QLabel(centralwidget);
        label_KI->setObjectName("label_KI");
        label_KI->setMaximumSize(QSize(16777215, 25));
        label_KI->setFont(font6);
        label_KI->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(label_KI);


        verticalLayout_14->addLayout(horizontalLayout_15);


        verticalLayout_13->addLayout(verticalLayout_14);

        horizontalLayout_PID_Inputs = new QHBoxLayout();
        horizontalLayout_PID_Inputs->setObjectName("horizontalLayout_PID_Inputs");
        horizontalLayout_KP = new QHBoxLayout();
        horizontalLayout_KP->setObjectName("horizontalLayout_KP");
        lineEdit_KP = new QLineEdit(centralwidget);
        lineEdit_KP->setObjectName("lineEdit_KP");

        horizontalLayout_KP->addWidget(lineEdit_KP);

        pushButton_SetKP = new QPushButton(centralwidget);
        pushButton_SetKP->setObjectName("pushButton_SetKP");
        pushButton_SetKP->setMinimumSize(QSize(0, 20));
        pushButton_SetKP->setFont(font2);

        horizontalLayout_KP->addWidget(pushButton_SetKP);


        horizontalLayout_PID_Inputs->addLayout(horizontalLayout_KP);

        horizontalLayout_KD = new QHBoxLayout();
        horizontalLayout_KD->setObjectName("horizontalLayout_KD");
        lineEdit_KD = new QLineEdit(centralwidget);
        lineEdit_KD->setObjectName("lineEdit_KD");

        horizontalLayout_KD->addWidget(lineEdit_KD);

        pushButton_SetKD = new QPushButton(centralwidget);
        pushButton_SetKD->setObjectName("pushButton_SetKD");
        pushButton_SetKD->setMinimumSize(QSize(0, 20));
        pushButton_SetKD->setFont(font2);

        horizontalLayout_KD->addWidget(pushButton_SetKD);


        horizontalLayout_PID_Inputs->addLayout(horizontalLayout_KD);

        horizontalLayout_KI = new QHBoxLayout();
        horizontalLayout_KI->setObjectName("horizontalLayout_KI");
        lineEdit_KI = new QLineEdit(centralwidget);
        lineEdit_KI->setObjectName("lineEdit_KI");

        horizontalLayout_KI->addWidget(lineEdit_KI);

        pushButton_SetKI = new QPushButton(centralwidget);
        pushButton_SetKI->setObjectName("pushButton_SetKI");
        pushButton_SetKI->setMinimumSize(QSize(0, 20));
        pushButton_SetKI->setFont(font2);

        horizontalLayout_KI->addWidget(pushButton_SetKI);


        horizontalLayout_PID_Inputs->addLayout(horizontalLayout_KI);


        verticalLayout_13->addLayout(horizontalLayout_PID_Inputs);


        verticalLayout_6->addLayout(verticalLayout_13);

        verticalLayout_PID_LINE = new QVBoxLayout();
        verticalLayout_PID_LINE->setObjectName("verticalLayout_PID_LINE");
        label_PID_LINE_title = new QLabel(centralwidget);
        label_PID_LINE_title->setObjectName("label_PID_LINE_title");
        label_PID_LINE_title->setMaximumSize(QSize(16777215, 30));
        label_PID_LINE_title->setFont(font4);
        label_PID_LINE_title->setAlignment(Qt::AlignCenter);

        verticalLayout_PID_LINE->addWidget(label_PID_LINE_title);

        horizontalLayout_PID_LINE_headers = new QHBoxLayout();
        horizontalLayout_PID_LINE_headers->setObjectName("horizontalLayout_PID_LINE_headers");
        label_PID_LINE_KP_hdr = new QLabel(centralwidget);
        label_PID_LINE_KP_hdr->setObjectName("label_PID_LINE_KP_hdr");
        label_PID_LINE_KP_hdr->setMaximumSize(QSize(16777215, 30));
        label_PID_LINE_KP_hdr->setFont(font);
        label_PID_LINE_KP_hdr->setAlignment(Qt::AlignCenter);

        horizontalLayout_PID_LINE_headers->addWidget(label_PID_LINE_KP_hdr);

        label_PID_LINE_KD_hdr = new QLabel(centralwidget);
        label_PID_LINE_KD_hdr->setObjectName("label_PID_LINE_KD_hdr");
        label_PID_LINE_KD_hdr->setMaximumSize(QSize(16777215, 30));
        label_PID_LINE_KD_hdr->setFont(font);
        label_PID_LINE_KD_hdr->setAlignment(Qt::AlignCenter);

        horizontalLayout_PID_LINE_headers->addWidget(label_PID_LINE_KD_hdr);

        label_PID_LINE_KI_hdr = new QLabel(centralwidget);
        label_PID_LINE_KI_hdr->setObjectName("label_PID_LINE_KI_hdr");
        label_PID_LINE_KI_hdr->setMaximumSize(QSize(16777215, 30));
        label_PID_LINE_KI_hdr->setFont(font);
        label_PID_LINE_KI_hdr->setAlignment(Qt::AlignCenter);

        horizontalLayout_PID_LINE_headers->addWidget(label_PID_LINE_KI_hdr);


        verticalLayout_PID_LINE->addLayout(horizontalLayout_PID_LINE_headers);

        horizontalLayout_PID_LINE_values = new QHBoxLayout();
        horizontalLayout_PID_LINE_values->setObjectName("horizontalLayout_PID_LINE_values");
        label_KP_LINE = new QLabel(centralwidget);
        label_KP_LINE->setObjectName("label_KP_LINE");
        label_KP_LINE->setMaximumSize(QSize(16777215, 25));
        label_KP_LINE->setFont(font5);
        label_KP_LINE->setAlignment(Qt::AlignCenter);

        horizontalLayout_PID_LINE_values->addWidget(label_KP_LINE);

        label_KD_LINE = new QLabel(centralwidget);
        label_KD_LINE->setObjectName("label_KD_LINE");
        label_KD_LINE->setMaximumSize(QSize(16777215, 25));
        label_KD_LINE->setFont(font5);
        label_KD_LINE->setAlignment(Qt::AlignCenter);

        horizontalLayout_PID_LINE_values->addWidget(label_KD_LINE);

        label_KI_LINE = new QLabel(centralwidget);
        label_KI_LINE->setObjectName("label_KI_LINE");
        label_KI_LINE->setMaximumSize(QSize(16777215, 25));
        label_KI_LINE->setFont(font5);
        label_KI_LINE->setAlignment(Qt::AlignCenter);

        horizontalLayout_PID_LINE_values->addWidget(label_KI_LINE);


        verticalLayout_PID_LINE->addLayout(horizontalLayout_PID_LINE_values);

        horizontalLayout_PID_LINE_inputs = new QHBoxLayout();
        horizontalLayout_PID_LINE_inputs->setObjectName("horizontalLayout_PID_LINE_inputs");
        horizontalLayout_KP_LINE = new QHBoxLayout();
        horizontalLayout_KP_LINE->setObjectName("horizontalLayout_KP_LINE");
        lineEdit_KP_LINE = new QLineEdit(centralwidget);
        lineEdit_KP_LINE->setObjectName("lineEdit_KP_LINE");

        horizontalLayout_KP_LINE->addWidget(lineEdit_KP_LINE);

        pushButton_SetKP_LINE = new QPushButton(centralwidget);
        pushButton_SetKP_LINE->setObjectName("pushButton_SetKP_LINE");
        pushButton_SetKP_LINE->setMinimumSize(QSize(0, 20));
        pushButton_SetKP_LINE->setFont(font2);

        horizontalLayout_KP_LINE->addWidget(pushButton_SetKP_LINE);


        horizontalLayout_PID_LINE_inputs->addLayout(horizontalLayout_KP_LINE);

        horizontalLayout_KD_LINE = new QHBoxLayout();
        horizontalLayout_KD_LINE->setObjectName("horizontalLayout_KD_LINE");
        lineEdit_KD_LINE = new QLineEdit(centralwidget);
        lineEdit_KD_LINE->setObjectName("lineEdit_KD_LINE");

        horizontalLayout_KD_LINE->addWidget(lineEdit_KD_LINE);

        pushButton_SetKD_LINE = new QPushButton(centralwidget);
        pushButton_SetKD_LINE->setObjectName("pushButton_SetKD_LINE");
        pushButton_SetKD_LINE->setMinimumSize(QSize(0, 20));
        pushButton_SetKD_LINE->setFont(font2);

        horizontalLayout_KD_LINE->addWidget(pushButton_SetKD_LINE);


        horizontalLayout_PID_LINE_inputs->addLayout(horizontalLayout_KD_LINE);

        horizontalLayout_KI_LINE = new QHBoxLayout();
        horizontalLayout_KI_LINE->setObjectName("horizontalLayout_KI_LINE");
        lineEdit_KI_LINE = new QLineEdit(centralwidget);
        lineEdit_KI_LINE->setObjectName("lineEdit_KI_LINE");

        horizontalLayout_KI_LINE->addWidget(lineEdit_KI_LINE);

        pushButton_SetKI_LINE = new QPushButton(centralwidget);
        pushButton_SetKI_LINE->setObjectName("pushButton_SetKI_LINE");
        pushButton_SetKI_LINE->setMinimumSize(QSize(0, 20));
        pushButton_SetKI_LINE->setFont(font2);

        horizontalLayout_KI_LINE->addWidget(pushButton_SetKI_LINE);


        horizontalLayout_PID_LINE_inputs->addLayout(horizontalLayout_KI_LINE);


        verticalLayout_PID_LINE->addLayout(horizontalLayout_PID_LINE_inputs);


        verticalLayout_6->addLayout(verticalLayout_PID_LINE);

        label_SETPOINT = new QLabel(centralwidget);
        label_SETPOINT->setObjectName("label_SETPOINT");
        label_SETPOINT->setBaseSize(QSize(0, 20));
        label_SETPOINT->setAlignment(Qt::AlignCenter);

        verticalLayout_6->addWidget(label_SETPOINT);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        spinBox_SETPOINT = new QDoubleSpinBox(centralwidget);
        spinBox_SETPOINT->setObjectName("spinBox_SETPOINT");
        spinBox_SETPOINT->setMinimum(-180.000000000000000);
        spinBox_SETPOINT->setMaximum(180.000000000000000);
        spinBox_SETPOINT->setSingleStep(0.250000000000000);

        horizontalLayout_5->addWidget(spinBox_SETPOINT);

        pushButton_SETPOINT = new QPushButton(centralwidget);
        pushButton_SETPOINT->setObjectName("pushButton_SETPOINT");
        pushButton_SETPOINT->setMinimumSize(QSize(0, 0));
        pushButton_SETPOINT->setMaximumSize(QSize(60, 16777215));

        horizontalLayout_5->addWidget(pushButton_SETPOINT);


        verticalLayout_6->addLayout(horizontalLayout_5);

        gridLayout_DirectionButtons = new QGridLayout();
        gridLayout_DirectionButtons->setObjectName("gridLayout_DirectionButtons");
        btn_UP = new QPushButton(centralwidget);
        btn_UP->setObjectName("btn_UP");
        btn_UP->setMinimumSize(QSize(45, 25));

        gridLayout_DirectionButtons->addWidget(btn_UP, 0, 1, 1, 1);

        btn_LEFT = new QPushButton(centralwidget);
        btn_LEFT->setObjectName("btn_LEFT");
        btn_LEFT->setMinimumSize(QSize(45, 25));

        gridLayout_DirectionButtons->addWidget(btn_LEFT, 1, 0, 1, 1);

        btn_RIGHT = new QPushButton(centralwidget);
        btn_RIGHT->setObjectName("btn_RIGHT");
        btn_RIGHT->setMinimumSize(QSize(45, 25));

        gridLayout_DirectionButtons->addWidget(btn_RIGHT, 1, 2, 1, 1);

        btn_DOWN = new QPushButton(centralwidget);
        btn_DOWN->setObjectName("btn_DOWN");
        btn_DOWN->setMinimumSize(QSize(45, 25));

        gridLayout_DirectionButtons->addWidget(btn_DOWN, 2, 1, 1, 1);


        verticalLayout_6->addLayout(gridLayout_DirectionButtons);


        splitter_main->addWidget(widget_controls);


        gridLayout->addWidget(splitter_main, 0, 0, 1, 1);

        pushButton = new QPushButton(centralwidget);
        pushButton->setObjectName("pushButton");
        pushButton->setMaximumSize(QSize(16777215, 30));

        gridLayout->addWidget(pushButton, 1, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1317, 28));
        menuDevice = new QMenu(menubar);
        menuDevice->setObjectName("menuDevice");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuDevice->menuAction());
        menuDevice->addAction(actionScanPorts);
        menuDevice->addAction(actionConnect_Device);
        menuDevice->addAction(actionDisconnect_Device);
        menuDevice->addSeparator();
        menuDevice->addAction(actionQuit);

        retranslateUi(MainWindow);

        tabWidget_Graficas->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "BalancinQT \342\200\224 Panel de Control", nullptr));
        actionScanPorts->setText(QCoreApplication::translate("MainWindow", "Scan Ports", nullptr));
        actionConnect_Device->setText(QCoreApplication::translate("MainWindow", "Connect Device", nullptr));
        actionDisconnect_Device->setText(QCoreApplication::translate("MainWindow", "Disconnect Device", nullptr));
        actionQuit->setText(QCoreApplication::translate("MainWindow", "Quit", nullptr));
        label_inclination_X->setText(QCoreApplication::translate("MainWindow", "Roll:", nullptr));
        label_inclination_Xvalue->setText(QCoreApplication::translate("MainWindow", "0.00 \302\260", nullptr));
        label_inclination_Y->setText(QCoreApplication::translate("MainWindow", "Pitch:", nullptr));
        label_inclination_Yvalue->setText(QCoreApplication::translate("MainWindow", "0.00 \302\260", nullptr));
        label_LogDir->setText(QCoreApplication::translate("MainWindow", "Log Dir:", nullptr));
        btnSelectLogDir->setText(QCoreApplication::translate("MainWindow", "\342\200\246", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "ACELERACI\303\223N", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "GIROSCOPIO", nullptr));
        tabWidget_Graficas->setTabText(tabWidget_Graficas->indexOf(tab_IMU), QCoreApplication::translate("MainWindow", "IMU & \303\201ngulos", nullptr));
        tabWidget_Graficas->setTabText(tabWidget_Graficas->indexOf(tab_PID), QCoreApplication::translate("MainWindow", "PID Control", nullptr));
        tabWidget_Graficas->setTabText(tabWidget_Graficas->indexOf(tab_Motors), QCoreApplication::translate("MainWindow", "Motores & PWM", nullptr));
        tabWidget_Graficas->setTabText(tabWidget_Graficas->indexOf(tab_System), QCoreApplication::translate("MainWindow", "Sistema", nullptr));
        textEdit_RAW->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /></head>"
"<body style=\" font-family:'Consolas'; font-size:12px; font-weight:400; font-style:normal; background-color:transparent;\">\n"
"<p align=\"center\" style=\"margin:12px 4px 8px 4px; padding:6px; background-color:rgba(20,50,8,0.85); border-radius:5px;\">"
"<span style=\"font-family:'Consolas'; font-size:13px; font-weight:bold; color:#78d878; letter-spacing:2px;\">&#x2191; DATO ENVIADO &#x2191;</span>"
"</p></body></html>", nullptr));
        textEdit_PROCCES->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /></head>"
"<body style=\" font-family:'Consolas'; font-size:13px; font-weight:400; font-style:normal; background-color:transparent;\">\n"
"<p align=\"center\" style=\"margin:12px 4px 8px 4px; padding:6px; background-color:rgba(0,50,35,0.85); border-radius:5px;\">"
"<span style=\"font-family:'Consolas'; font-size:13px; font-weight:bold; color:#44ffcc; letter-spacing:2px;\">&#x2193; DATO RECIBIDO &#x2193;</span>"
"</p></body></html>", nullptr));
        pushButton_SEND->setText(QCoreApplication::translate("MainWindow", "Send CMD", nullptr));
        pushButton_BALANCE->setText(QCoreApplication::translate("MainWindow", "BALANCE", nullptr));
        pushButton_FOLLOW_LINE->setText(QCoreApplication::translate("MainWindow", "FOLLOW LINE", nullptr));
        pushButton_UDP->setText(QCoreApplication::translate("MainWindow", "Send UDP", nullptr));
        pushButton_RECORD->setText(QCoreApplication::translate("MainWindow", "\342\217\272 RECORD", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "Local PORT:", nullptr));
        lineEdit_LOCALPORT->setText(QCoreApplication::translate("MainWindow", "30010", nullptr));
        pushButton_OPENUDP->setText(QCoreApplication::translate("MainWindow", "Open UDP", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "Device IP:", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "Device PORT:", nullptr));
        lineEdit_DEVICEPORT->setText(QString());
        label->setText(QCoreApplication::translate("MainWindow", "PID BALANCE", nullptr));
        label_13->setText(QCoreApplication::translate("MainWindow", "KP", nullptr));
        label_14->setText(QCoreApplication::translate("MainWindow", "KD", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "KI", nullptr));
        label_KP->setText(QCoreApplication::translate("MainWindow", "0.000", nullptr));
        label_KD->setText(QCoreApplication::translate("MainWindow", "0.000", nullptr));
        label_KI->setText(QCoreApplication::translate("MainWindow", "0.000", nullptr));
        lineEdit_KP->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nuevo KP", nullptr));
        pushButton_SetKP->setText(QCoreApplication::translate("MainWindow", "SET", nullptr));
        lineEdit_KD->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nuevo KD", nullptr));
        pushButton_SetKD->setText(QCoreApplication::translate("MainWindow", "SET", nullptr));
        lineEdit_KI->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nuevo KI", nullptr));
        pushButton_SetKI->setText(QCoreApplication::translate("MainWindow", "SET", nullptr));
        label_PID_LINE_title->setText(QCoreApplication::translate("MainWindow", "PID SEGUIDOR L\303\215NEA", nullptr));
        label_PID_LINE_KP_hdr->setText(QCoreApplication::translate("MainWindow", "KP", nullptr));
        label_PID_LINE_KD_hdr->setText(QCoreApplication::translate("MainWindow", "KD", nullptr));
        label_PID_LINE_KI_hdr->setText(QCoreApplication::translate("MainWindow", "KI", nullptr));
        label_KP_LINE->setText(QCoreApplication::translate("MainWindow", "0.000", nullptr));
        label_KD_LINE->setText(QCoreApplication::translate("MainWindow", "0.000", nullptr));
        label_KI_LINE->setText(QCoreApplication::translate("MainWindow", "0.000", nullptr));
        lineEdit_KP_LINE->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nuevo KP", nullptr));
        pushButton_SetKP_LINE->setText(QCoreApplication::translate("MainWindow", "SET", nullptr));
        lineEdit_KD_LINE->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nuevo KD", nullptr));
        pushButton_SetKD_LINE->setText(QCoreApplication::translate("MainWindow", "SET", nullptr));
        lineEdit_KI_LINE->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nuevo KI", nullptr));
        pushButton_SetKI_LINE->setText(QCoreApplication::translate("MainWindow", "SET", nullptr));
        label_SETPOINT->setText(QCoreApplication::translate("MainWindow", "\342\246\277 Setpoint", nullptr));
        pushButton_SETPOINT->setText(QCoreApplication::translate("MainWindow", "SET", nullptr));
        btn_UP->setText(QCoreApplication::translate("MainWindow", "\342\226\262", nullptr));
        btn_LEFT->setText(QCoreApplication::translate("MainWindow", "\342\227\200", nullptr));
        btn_RIGHT->setText(QCoreApplication::translate("MainWindow", "\342\226\266", nullptr));
        btn_DOWN->setText(QCoreApplication::translate("MainWindow", "\342\226\274", nullptr));
        pushButton->setText(QCoreApplication::translate("MainWindow", "CLEAR DISPLAYS", nullptr));
        menuDevice->setTitle(QCoreApplication::translate("MainWindow", "Device", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
