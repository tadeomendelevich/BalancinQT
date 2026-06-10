<div align="center">

# 🤖 BalancinQT — Interfaz de Control y Monitoreo

**Aplicación de escritorio Qt para el robot autobalanceado STM32**

[![C++](https://img.shields.io/badge/C++-Qt%20Framework-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](https://www.qt.io/)
[![Qt](https://img.shields.io/badge/Qt-Widgets%20%2B%20Charts-41CD52?style=for-the-badge&logo=qt&logoColor=white)](https://doc.qt.io/)
[![STM32](https://img.shields.io/badge/STM32-Firmware-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white)](https://www.st.com/)
[![Serial/UDP](https://img.shields.io/badge/Comm-Serial%20%2F%20UDP-0078D7?style=for-the-badge)]()

*Universidad Nacional de Entre Ríos (UNER)*

</div>

---

## 📸 Interfaz gráfica

<p align="center">
  <img src="interfaz_QT.png" width="750" alt="Interfaz Qt del robot autobalanceado"/>
</p>

---

## 📋 Descripción

Aplicación de escritorio desarrollada en **C++ con Qt** para el monitoreo, control y análisis en tiempo real del robot autobalanceado ([ver firmware](https://github.com/tadeomendelevich/Balancin_Mendelevich)). Facilita el ajuste de parámetros de control, la visualización de sensores y el análisis del comportamiento dinámico del sistema.

La comunicación con el hardware se realiza mediante un **protocolo binario propio (UNER)** por USB-Serial o WiFi (ESP-01/UDP).

---

## ✨ Funcionalidades

### 🎛️ Control
- Ajuste de parámetros **PID** (Kp, Ki, Kd) en tiempo real
- Control manual por **teclado** (flechas) y **botones D-PAD** en pantalla
- Activación de modos de operación:
  - 🔄 Balanceo automático
  - ➖ Seguidor de línea
  - 🕹️ Control manual

### 📊 Visualización (8 pestañas en tiempo real)

| Pestaña | Contenido |
|---|---|
| Aceleración / Ángulo | Fusión de sensores — roll y pitch |
| Giroscopio | Datos raw y filtrados |
| Control PID | P, I, D, error, output |
| Motores / PWM | Señales de potencia |
| Sistema | Tiempos de control y flags |
| Sensores raw | Acelerómetro + giroscopio sin procesar |
| ADC | 8 canales analógicos |
| Seguidor de línea | PID + valores de sensores IR |

### 💾 Logging
- Registro de sesión en archivos **CSV** con timestamp automático
- Compatible con Excel, MATLAB, Python para análisis posterior

---

## 🔌 Protocolo de comunicación UNER

```
┌────────┬────────────────┬──────────┐
│ Header │    Payload     │ Checksum │
│ "UNER" │ Datos variables│  1 byte  │
└────────┴────────────────┴──────────┘
```

Soporta dos canales de comunicación:
- **USB-Serial** — `QSerialPort`
- **WiFi UDP** — `QUdpSocket` vía módulo ESP-01

---

## 🛠️ Compilación

### Requisitos

- Qt 5.x o superior (módulos: Widgets, Charts, SerialPort, Network)
- Compilador C++17 (MinGW o MSVC en Windows, GCC en Linux)

### Build

```bash
# Con Qt Creator: abrir BalancinQT.pro y compilar
# O desde línea de comandos:
qmake BalancinQT.pro
make
```

---

## 📁 Estructura del proyecto

```
BalancinQT/
├── main.cpp              # Punto de entrada
├── mainwindow.cpp/h      # Ventana principal y lógica de UI
├── mainwindow.ui         # Definición de la interfaz gráfica
├── robotviewer3d.cpp/h   # Visualizador 3D del robot
├── settingsdialog.cpp/h  # Diálogo de configuración de puerto
├── resources.qrc         # Recursos embebidos
└── formato_csv_telemetria.txt  # Especificación del formato de log
```

---

## 🔗 Proyectos relacionados

- **[Balancin_Mendelevich](https://github.com/tadeomendelevich/Balancin_Mendelevich)** — Firmware STM32 del robot (C, BlackPill STM32F411)
- **[-Blink2025](https://github.com/tadeomendelevich/-Blink2025)** — Plataforma de sensores STM32F103

---

<div align="center">

**Autor:** [Tadeo Mendelevich](https://github.com/tadeomendelevich)  
*Ingeniería en Sistemas de Información — UNER*

</div>
