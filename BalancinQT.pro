QT       += core gui serialport network widgets charts 3dcore 3drender 3dinput 3dextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    robotviewer3d.cpp \
    settingsdialog.cpp

HEADERS += \
    mainwindow.h \
    robotviewer3d.h \
    settingsdialog.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Copia automática de modelos 3D al directorio de salida
copy3dmodels.commands = \
    $$QMAKE_COPY \"$$shell_path($$PWD/AutoMicro.obj)\" \"$$shell_path($$OUT_PWD/release/)\" && \
    $$QMAKE_COPY \"$$shell_path($$PWD/AutoMicro.mtl)\" \"$$shell_path($$OUT_PWD/release/)\" && \
    $$QMAKE_COPY \"$$shell_path($$PWD/AutoMicro.obj)\" \"$$shell_path($$OUT_PWD/debug/)\" && \
    $$QMAKE_COPY \"$$shell_path($$PWD/AutoMicro.mtl)\" \"$$shell_path($$OUT_PWD/debug/)\"
first.depends += copy3dmodels
QMAKE_EXTRA_TARGETS += copy3dmodels first

RESOURCES += \
    resources.qrc
