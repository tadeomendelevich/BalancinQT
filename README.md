# 🖥️ Interfaz de Control y Monitoreo - Self-Balancing Robot
Aplicación desarrollada en Qt para el monitoreo, control y análisis en tiempo real de un robot autobalanceado basado en STM32.

---

## 📸 Interfaz gráfica

<p align="center">
  <img src="assets/interfaz_QT.png" width="700"/>
</p>
---

## 🧠 Descripción

Esta aplicación permite interactuar en tiempo real con el sistema embebido del robot autobalanceado, facilitando el monitoreo de variables críticas, ajuste de parámetros de control y análisis del comportamiento dinámico del sistema.

Integra comunicación serial y UDP con un protocolo propio, visualización de datos en gráficos y herramientas de logging para análisis posterior.

---

## ⚙️ Tecnologías utilizadas

- C++  
- Qt (Qt Widgets + Qt Charts)  
- QSerialPort (comunicación serial)  
- QUdpSocket (comunicación UDP)  
- Protocolo de comunicación UNER  
- Microcontrolador STM32  

---

## 🚀 Funcionalidades principales

- 📡 Comunicación en tiempo real (Serial y UDP)  
- 📊 Visualización de datos en múltiples gráficos dinámicos  
- 🎛️ Ajuste de parámetros PID (KP, KI, KD) en vivo  
- 📁 Registro de datos en archivos CSV  
- 🎮 Control manual del robot (teclado y botones)  
- 🔄 Monitoreo de sensores (MPU6050 y ADCs)  
- 🤖 Activación de modos:
  - Balanceo automático  
  - Seguidor de línea  
  - Control manual  

---

## 📊 Visualización de datos

La interfaz incluye múltiples pestañas para análisis en tiempo real:

- 📈 Aceleración y ángulo (fusión de sensores)  
- 🌀 Giroscopio (raw y filtrado)  
- ⚙️ Control PID (P, I, D, error, output)  
- 🔋 Motores y señales PWM  
- ⏱️ Sistema (tiempos de control y flags)  
- 📡 Sensores raw (acelerómetro y giroscopio)  
- 📊 Valores ADC (8 canales)  
- ➖ Seguidor de línea (PID + sensores)  

---

## 🔌 Comunicación

El sistema utiliza un protocolo propio basado en paquetes estructurados:

- Header: `UNER`  
- Payload dinámico  
- Checksum de validación  

Soporta comunicación por:
- Serial (USB)  
- UDP (WiFi - ESP01)  

---

## 🎮 Control del sistema

- Control manual mediante teclado (flechas)  
- Control mediante interfaz gráfica (botones tipo D-PAD)  
- Envío de comandos para modificar parámetros en tiempo real  

---

## 📁 Logging

- Registro de datos en archivos `.csv`  
- Exportación automática con timestamp  
- Compatible con análisis en herramientas externas  

---

## 🧪 Desafíos técnicos

- Manejo de comunicación en tiempo real  
- Sincronización de datos entre hardware y GUI  
- Visualización eficiente de múltiples señales  
- Implementación de protocolo robusto  
- Gestión de jitter y tiempos de muestreo  

---

## 🎯 Objetivo

Desarrollar una herramienta de monitoreo y control que permita analizar, ajustar y optimizar el comportamiento del robot autobalanceado en tiempo real.

---

## 👨‍💻 Autor

Tadeo Mendelevich  
https://github.com/tadeomendelevich
