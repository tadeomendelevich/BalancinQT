# Balancin Qt — Memoria del Proyecto
> Este archivo es la memoria persistente del proyecto para Claude Code.
> Mantenerlo actualizado al final de cada sesión de trabajo.

---

## 🤖 Instrucciones Permanentes para Claude

**Estas reglas aplican en cada sesión, automáticamente y sin que te lo pida:**

### Al modificar cualquier archivo de código:
- Agregá una fila en "Registro de Cambios" con fecha actual (YYYY-MM-DD),
  archivo(s) tocado(s), qué cambió y por qué
- Si el cambio afecta parámetros PID, protocolo UART/UDP u otra sección
  del CLAUDE.md → actualizá esa sección también
- Si resolviste un bug → pasalo de "Pendientes" a "Funcionalidades completas"
- Si apareció un bug nuevo → agregalo en "Pendientes / bugs conocidos"
- Si tomaste una decisión de diseño importante → agregala en "Decisiones de Diseño"

### Al iniciar sesión:
- Leé este CLAUDE.md completo antes de hacer cualquier cosa
- Usalo como contexto del proyecto, no preguntes lo que ya está documentado acá

### Si un cambio afecta al otro proyecto (Qt ↔ STM32):
- Avisame explícitamente qué hay que cambiar en el otro proyecto
- Indicá el archivo exacto que necesita modificación

**Nunca rompas la estructura de este CLAUDE.md.**

---

## Descripción General
Interfaz de escritorio en **Qt** para monitoreo y control del péndulo invertido "Balancín Mendelevich".
Permite visualizar el estado en tiempo real, ajustar parámetros de control PID y comunicarse con
el firmware STM32 por dos canales: UART/Serial y WiFi UDP.

---

## Rutas del Proyecto
| Proyecto | Ruta |
|----------|------|
| Qt (este) | `C:\Microcontroladores\BalancinQT` |
| STM32 (firmware) | `C:\Users\tadeo\STM32CubeIDE\workspace_1.18.1\Balancin_Mendelevich` |

---

## Arquitectura de Archivos
```
BalancinQT/
├── CLAUDE.md                  ← memoria del proyecto (este archivo)
├── BalancinQT.pro             ← proyecto qmake (Qt 6, C++17)
├── resources.qrc              ← recursos embebidos
├── main.cpp                   ← entry point, aplica estilo Fusion
├── mainwindow.cpp / .h        ← ventana principal: protocolo, gráficos, UI
├── mainwindow.ui              ← layout UI generado por Qt Designer
├── odomchartview.cpp / .h     ← QChartView con pan (arrastre) + zoom (rueda, centrado en cursor) para el mapa de odometría (2026-07-06)
├── healthdashboard.cpp / .h   ← panel de salud flotante (conexión/paquetes/latencia/pérdida/modo/alarma) (2026-07-06)
├── robotviewer3d.cpp / .h     ← visor 3D del robot (Qt3D, modelo OBJ)
├── settingsdialog.cpp / .h    ← diálogo de configuración del puerto serial
├── settingsdialog.ui          ← layout del diálogo serial
├── AutoMicro.obj / .mtl       ← modelo 3D del robot exportado de Fusion 360
└── ui_mainwindow.h / ui_settingsdialog.h  ← generados automáticamente por uic
```
> Nota (actualizada 2026-07-06): el grueso de la lógica (protocolo, parsing, casi todos los gráficos)
> sigue todo en `mainwindow.cpp`, pero ya no es 100% monolítico — `OdomChartView` y `HealthDashboard`
> se separaron a sus propios archivos por ser widgets reutilizables con estado/comportamiento propio
> (ver "Decisiones de Diseño"). Si se agregan más widgets con lógica propia, seguir ese mismo patrón.

---

## Comunicación con STM32
### Protocolo "UNER" — compartido por Serial y UDP
Ambos canales usan el mismo formato de trama binaria:

```
[U][N][E][R][NBYTES][':'][CMD][DATA...][CHK]
 ^--- Header 4 bytes  ^    ^   ^         ^
                      |    |   |         XOR acumulado de todos los bytes anteriores
                      |    |   Payload: código de comando + datos opcionales
                      |    Token separador (0x3A)
                      Cantidad de bytes del payload incluido el checksum final
```

- **Checksum:** XOR de `U^N^E^R^NBYTES^':'` XOR cada byte del payload (sin incluir el CHK)
- **Valores float** se envían como 4 bytes little-endian (IEEE 754)
- **Valores int** se envían como 4 bytes little-endian (int32)

### Canal 1 — UART/Serial
| Campo | Valor |
|-------|-------|
| Baudrate por defecto | **115200** (seleccionable: 9600 / 19200 / 38400 / 115200 / Custom) |
| Bits de datos | 8 |
| Paridad | None |
| Bits de parada | 1 |
| Control de flujo | None |
| Puerto COM | Seleccionable en menú Device → Scan Ports |

**Nota:** para comandos simples (sin payload extra) el campo NBYTES se setea a 0x02 en Serial — posible bug si el firmware espera el valor dinámico como en UDP.

### Canal 2 — WiFi UDP
| Campo | Valor |
|-------|-------|
| IP del STM32 | Dinámica — se aprende del primer datagrama recibido (auto-descubrimiento) |
| Puerto local Qt (RX) | Ingresado por el usuario en `lineEdit_LOCALPORT` |
| Puerto remoto STM32 | Aprendido automáticamente del sender del primer paquete UDP |
| Formato de paquete | Igual al protocolo UNER descrito arriba |
| Ping inicial | Envía byte `'r'` (0x72) al abrir el socket si ya hay IP/puerto configurados |
| Watchdog | Timer 1000 ms — limpia pantallas si no llegan datos por 5 s |

**Comandos principales:**
| Comando | Código | Payload enviado Qt→STM32 | Descripción |
|---------|--------|--------------------------|-------------|
| GETALIVE | 0xF0 | — | Ping, STM32 responde ACK (0x0D) |
| GETFIRMWARE | 0xF1 | — | STM32 responde string de versión |
| GETADCVALUES | 0xA5 | — | Solicita 8 valores ADC (uint16 cada uno) |
| SETMOTORSPEED | 0xA1 | int32 motor1 + int32 motor2 (-100..100) | Velocidad directa de motores |
| SETSERVOANGLE | 0xA2 | int8 ángulo | Posición del servo |
| GETMPU6050VALUES | 0xA6 | — | Solicita ax,ay,az,gx,gy,gz (int16 c/u) |
| GETANGLE | 0xA7 | — | Solicita roll + pitch (float32 c/u) |
| SENDALLSENSORS | 0xA9 | — | Solicita ADC×8 + IMU×6 + roll/pitch (int16) |
| STOPALLSENSORS | 0xAA | — | Detiene envío periódico de sensores |
| MODIFYKP | 0xB1 | float32 Kp | STM32 responde con KP+KD+KI actuales |
| MODIFYKD | 0xB2 | float32 Kd | STM32 responde con KP+KD+KI actuales |
| MODIFYKI | 0xB3 | float32 Ki | STM32 responde con KP+KD+KI actuales |
| BALANCE | 0xB4 | — | Toggle activar/desactivar control de balance |
| RESETMASSCENTER | 0xB7 | — | Resetea el punto de equilibrio |
| ACTIVATE_CSV_LOG | 0xB9 | — | Toggle log CSV en el STM32 |
| ACTIVATE_WIFI_LOG | 0xBA | — | Toggle envío de telemetría WiFi (WIFI_LOG_DATA) |
| WIFI_LOG_DATA | 0xBB | — | **STM32→Qt**: struct `WifiLogData_t` (~60 bytes) |
| MODIFY_BETA_G | 0xBC | float32 | Factor de fusión giroscopio |
| MODIFY_BETA_A | 0xBD | float32 | Factor de fusión acelerómetro |
| CHANGE_DISPLAY | 0xBE | — | Cambia página del display del robot |
| MODIFY_KV_BRAKE | 0xBF | float32 | Ganancia de frenado dinámico |
| MODIFY_KP_LINE | 0xC0 | float32 | Kp del seguidor de línea |
| MODIFY_KD_LINE | 0xC1 | float32 | Kd del seguidor de línea |
| MODIFY_KI_LINE | 0xC2 | float32 | Ki del seguidor de línea |
| MODIFY_LINE_THRES | 0xC3 | float32 | Umbral de detección de línea |
| MODIFY_LINE_SPEED | 0xC4 | float32 (grados) | Ángulo de avance del seguidor de línea |
| ACTIVATE_LINE_FOLLOWING | 0xC5 | — | Toggle seguidor de línea |
| ACTIVATE_POS_MAINTENANCE | 0xC6 | — | Toggle mantenimiento de posición |
| ACTIVATE_MANUAL_CONTROL | 0xC7 | — | Toggle modo control manual |
| MODIFY_SETPOINT | 0xC8 | float32 (grados) | Ángulo de equilibrio objetivo |
| MOVE_FORWARD | 0xD0 | — | Mover adelante (manual, se repite a 20 Hz) |
| MOVE_BACKWARD | 0xD1 | — | Mover atrás |
| MOVE_LEFT | 0xD2 | — | Girar izquierda |
| MOVE_RIGHT | 0xD3 | — | Girar derecha |
| MOVE_STOP | 0xD4 | — | Detener movimiento |
| GET_ODOMETRY | 0xDA | — | Pide la pose actual; STM32 responde 3 floats32 (x[m], y[m], theta[°]) |
| RESET_ODOMETRY | 0xDB | — | Resetea la pose a (0,0,0°); STM32 responde ACK |
| WIFI_ODOM_DATA | 0xDC | — | **STM32→Qt, push automático (no se pide)**: struct `WifiOdomData_t`, cada 500ms mientras haya WiFi conectado — no requiere `ACTIVATE_WIFI_LOG` |

**Estructura `WifiLogData_t`** (recibida vía UDP, ~60 bytes, packed):
```
t_ms (uint32) | roll_filt (float) | output (float) | p_term (float) | i_term (float) | d_term (float)
mR (int16) | mL (int16) | dt_ctrl_us (uint32) | dyn_sp (float)
line_error (float) | p_line (float) | i_line (float) | d_line (float) | steering_adjustment (float)
adc1..adc4 (uint16 cada uno)
```

**Estructura `WifiOdomData_t`** (recibida vía UDP, push automático cada 500ms, packed):
```
seq (uint16) | t_ms (uint32) | x_m (float) | y_m (float) | theta_deg (float)
line_error (float) | line_detected (uint8) | robot_state (uint8) | line_state (uint8)
```
`seq` es incremental (para detectar pérdida de paquetes contando huecos). `robot_state`/`line_state`
son mirrors de los enums `eRobotState`/`eLineState` del firmware — frágiles ante reordenamientos,
ver comentario `ODOM_LINE_STATE_OBJ_FIRST` en `mainwindow.h`.

**Telemetría CSV** recibida por Serial (26+ columnas, valores escalados por ×1000 o ×100):
```
t_ms, dt_us, dt_ctrl_us, accel_roll×1000, accel_roll_f×1000, gyro_y×1000, gyro_f×1000,
roll_filt×1000, dyn_sp×1000, error×1000, p×1000, i×1000, d×1000, output×1000,
pwm_cmd×100, pwm_sat×100, sat_flag, mR, mL, pitch×1000, ax×100, ay×100, az×100, gx×100, gy×100, gz×100
```

---

## Parámetros del Control PID
Parámetros que Qt puede leer y modificar en el STM32:
| Parámetro | Descripción | Valor actual |
|-----------|-------------|--------------|
| Kp | Ganancia proporcional | No hardcodeado en Qt — se lee desde STM32 al responder MODIFYKP/KD/KI |
| Ki | Ganancia integral | No hardcodeado en Qt — ídem |
| Kd | Ganancia derivativa | No hardcodeado en Qt — ídem |
| Setpoint | Ángulo de equilibrio objetivo (°) | Ingresado por usuario en `spinBox_SETPOINT` (float) |
| Deadband | Zona muerta del actuador | No expuesto en Qt — solo en firmware STM32 |
| Beta_G | Factor de fusión giroscopio | Configurable vía 0xBC (rango 0..10) |
| Beta_A | Factor de fusión acelerómetro | Configurable vía 0xBD (rango 0..10) |
| Kv_brake | Ganancia de frenado dinámico | Configurable vía 0xBF (rango 0..100) |
| Kp_line | Ganancia proporcional seguidor línea | Configurable vía 0xC0 (rango 0..100) |
| Ki_line | Ganancia integral seguidor línea | Configurable vía 0xC2 (rango 0..10) |
| Kd_line | Ganancia derivativa seguidor línea | Configurable vía 0xC1 (rango 0..100) |
| Line_thres | Umbral detección de línea | Configurable vía 0xC3 (rango 0..10) |
| Line_speed | Ángulo de avance del seguidor | Configurable vía 0xC4 en grados (rango -100..100) |

---

## Estado Actual
- **Etapa:** Casi terminado
- **Última sesión:** 2026-07-06

### Funcionalidades completas ✅
- **Odometría**: comandos `GET_ODOMETRY`/`RESET_ODOMETRY` (combo de comandos) + push automático `WIFI_ODOM_DATA` cada 500ms. Pestaña "Odometría (WiFi)": mapa XY navegable (rueda=zoom centrado en cursor, arrastre=paneo, doble click/botón=reset), cinta de línea gruesa y negra, anotaciones de "línea perdida"/"objeto" en el mapa, flecha de rumbo en la posición actual, botón "Borrar mapa" (2026-07-06)
- **Dashboard de salud** (`HealthDashboard`), embebido en el panel de controles debajo de Device PORT (no flotante — la primera versión flotante se sacó por tapar la UI): "MODO: X" destacado arriba (grande, color por modo), y compacto debajo: estado de conexión WiFi, paquetes recibidos + tasa, pérdida de paquetes (vía `seq` de `WIFI_ODOM_DATA`), latencia (ping/pong con `GETALIVE` cada 2s), tiempo desde el último dato, última alarma/evento
- Panel "Dato enviado" (`textEdit_RAW`) oculto por defecto, con botón tipo tab para mostrarlo/ocultarlo — solo "Dato recibido" queda visible de entrada (2026-07-06)
- Protocolo UNER binario con checksum XOR, funciona en ambos canales (Serial y UDP)
- Comunicación Serial: conectar/desconectar, configurar puerto desde SettingsDialog
- Comunicación UDP: bind local, auto-descubrimiento de IP/puerto del STM32, watchdog de inactividad
- Recepción y visualización de telemetría CSV via Serial (26 columnas, valores escalados)
- Recepción y visualización de telemetría binaria UDP via `WIFI_LOG_DATA` (struct `WifiLogData_t`)
- Gráficos en tiempo real con ventana deslizante de 10 s (QtCharts): IMU, PID, Motores, Sistema, Sensores Raw, Seguidor de Línea (ADC + PID de línea)
- Visor 3D del robot con modelo OBJ de Fusion 360 (Qt3D), se inclina según roll_filt
- Toggle BALANCE (0xB4) con retroalimentación visual (botón verde/rojo)
- Toggle FOLLOW LINE (0xC5) con retroalimentación visual
- Control manual por D-PAD: botones en UI + teclas WASD/flechas, se repite a 20 Hz mientras se mantiene presionado
- Modificación en tiempo real de Kp, Ki, Kd, Setpoint, Beta_G, Beta_A, Kv_brake
- Modificación de parámetros del seguidor de línea (Kp/Ki/Kd_line, thres, speed)
- Grabación de CSV local en `C:/Users/tadeo/Desktop/Balancin_logs` (seleccionable)
- Activar/desactivar log CSV y WiFi Log en el STM32
- Reset del centro de masa, cambio de página del display del robot
- Labels numéricos en vivo del MPU crudo (Ax/Ay/Az/Gx/Gy/Gz) en tab dedicado
- Estilo Fusion de Qt aplicado globalmente para consistencia cross-platform

### Pendientes / bugs conocidos 🔧
- **Bug Serial NBYTES:** en `sendDataSerial()` los comandos simples hardcodean `NBYTES = 0x02` en lugar de calcularlo dinámicamente como hace `sendDataUDP()`. Puede causar problemas si el firmware valida el campo.
- **CSV grabado incompleto desde WIFI_LOG_DATA:** varios campos se escriben como `0` (❌): dt_us, accel_roll, accel_roll_f, gyro_y, gyro_f, error, pwm_cmd, pwm_sat, sat_flag, pitch, ax/ay/az, gx/gy/gz — solo están disponibles en la trama CSV Serial, no en la struct UDP.
- **`on_pushButton_released()`** tiene cuerpo vacío — botón sin acción definida.
- **Directorio de guardado hardcodeado** a `C:/Users/tadeo/Desktop/Balancin_logs` en el constructor — debería leerse de QSettings o similar.
- **`seriesAy` y `seriesAz`** se crean en el constructor pero nunca se agregan al `accChart` ni se attachan a ejes (código comentado). Solo se usan en el tab de sensores raw.
- **Protocolo CSV espera exactamente 26 columnas** (`parts.size() < 26`) — si el firmware agrega columnas sin actualizar este check, las líneas se descartan silenciosamente.

---

## 📋 Registro de Cambios
> **Instrucción para Claude:** Al finalizar cada sesión, agregar una fila con los cambios realizados.

| Fecha | Archivo(s) modificado(s) | Cambio realizado | Motivo / Observación |
|-------|--------------------------|------------------|----------------------|
| 2026-07-06 | mainwindow.cpp | **Paneles PID BALANCE / PID SEGUIDOR LÍNEA plegables**: ambos arrancan colapsados (solo título + botón "▾ Ver PID Balance"/"▾ Ver PID Línea") y se despliegan al tocar el botón. Implementado sin tocar el `.ui` — los widgets de cada sección (headers KP/KD/KI, valores, lineEdits+botones SET) ya eran items directos de layouts nombrados (`ui->verticalLayout_13` para Balance, `ui->verticalLayout_PID_LINE` para Línea, ambos expuestos por `uic`); un lambda `makePidFoldable` los oculta/muestra todos juntos e inserta el botón toggle en el índice 1 de cada layout (justo debajo del título). Con los widgets ocultos, el `QVBoxLayout` no les reserva espacio — la sección colapsada ocupa casi nada. | El usuario reportó que el panel de controles quedó muy apretado tras embeber el dashboard de salud, pidió poder plegar también los PID (Balance y Línea) para que todo entre bien |
| 2026-07-06 | healthdashboard.h/.cpp, mainwindow.h/.cpp | **Dashboard de salud reubicado + rediseñado compacto** (reemplaza la primera versión flotante arriba a la derecha, que tapaba la UI). Ahora se crea como hijo directo de `MainWindow` pero se **embebe** con `ui->verticalLayout->addWidget(healthDashboard)` — el mismo layout que ya tenía las filas Local PORT / Device IP / Device PORT, quedando justo debajo de esta última. Se eliminó el `resizeEvent` override (ya no hace falta reposicionar nada a mano). Rediseño visual: **"MODO: X" pasó a ser lo más prominente** (fuente 11pt bold, centrado, con color de fondo distinto por modo — gris IDLE, verde BALANCE/BALANCE+VEL, azul SEGUIDOR LINEA, naranja MANUAL, violeta TEST MOTORES) arriba de todo el panel; el resto de las estadísticas (paquetes, tasa, latencia, pérdida, último dato, alarma) se comprimieron a fuente 8pt con textos abreviados ("Pkts:", "Lat:", etc.) en una grilla 2 columnas para ocupar el mínimo espacio vertical posible. | El usuario reportó que el panel flotante "molestaba a la vista" y tapaba la UI, pidió reubicarlo junto a los controles de conexión (debajo de Device PORT) con elementos más chicos, y remarcó que le faltaba destacar el modo actual del robot (ya estaba implementado — `onRobotState()` — pero perdido entre el resto de las estadísticas sin jerarquía visual) |
| 2026-05-04 | CLAUDE.md | Lectura inicial completa del proyecto | Se completaron todas las secciones vacías con información real del código |
| 2026-07-06 | mainwindow.h, mainwindow.cpp | **Fix de compilación en `odomchartview.cpp`**: `QWheelEvent::pos()` no existe en la versión de Qt6 de este proyecto (fue eliminado, no solo deprecado, a diferencia de `QMouseEvent::pos()` que solo tira warning). Reemplazado por `event->position()` (QWheelEvent) y `event->position().toPoint()` (QMouseEvent, por prevención, mismo patrón de eventos en Qt6). | El usuario reportó error de compilación tras agregar `OdomChartView`; confirma que el proyecto usa Qt6 |
| 2026-07-06 | mainwindow.h, mainwindow.cpp | **Flecha de rumbo + botón "Borrar mapa" + mapa sin grilla/labels** (mejora sobre el mapa navegable recién agregado). (1) **Flecha de rumbo**: dos `QGraphicsItem` sueltos (`odomHeadingLine`/`odomHeadingArrowHead`, no forman parte de ninguna serie) que se dibujan en la posición actual apuntando en la dirección de `theta` (misma convención `cos(θ)`/`sin(θ)` que usa el firmware para integrar la odometría); largo proporcional al rango visible de los ejes (no fijo en metros) para que se vea igual de bien zoomeado o alejado; cabezal en forma de triángulo calculado en coordenadas de pantalla. `updateHeadingArrow()` se llama desde `updateOdomChart()` (dato nuevo) y desde `repositionOdomAnnotations()` (pan/zoom), usando la última pose guardada (`odomLastX/Y/Theta`). (2) **`clearOdomMap()`**: nuevo botón "Borrar mapa" junto al de reset de vista — vacía trayectoria, segmentos de cinta (los borra del chart y libera memoria), marcadores de eventos, anotaciones de texto (removidas de la escena y liberadas), reinicia el estado de detección de transiciones y el rango de ejes a los valores iniciales. (3) **Mapa limpio**: se sacaron los títulos de eje ("X [m]"/"Y [m]"), la grilla (mayor y menor), los números de eje y la línea del eje (`setTitleText("")`, `setGridLineVisible(false)`, `setMinorGridLineVisible(false)`, `setLabelsVisible(false)`, `setLineVisible(false)` en ambos ejes) — los ejes siguen existiendo como objetos internos (definen la escala/rango que usa `mapToPosition()`), pero no se dibuja nada de ellos; solo quedan visibles la cinta, la trayectoria, los marcadores y la flecha. Se agregó `#include <QtMath>`/`<cmath>`/`<algorithm>` a `mainwindow.cpp` para `qDegreesToRadians`/`std::cos`/`std::sin`/`std::atan2`/`std::max`. | Usuario pidió una flecha que indique hacia dónde apunta el robot, la capacidad de borrar el mapa completo, y un mapa más limpio sin grilla ni labels de eje, dejando solo las líneas del recorrido |
| 2026-07-06 | odomchartview.h/.cpp (nuevos), mainwindow.h, mainwindow.cpp, BalancinQT.pro | **Mapa de odometría navegable + cinta gruesa + anotaciones de eventos** (mejora sobre el gráfico XY agregado más temprano el mismo día). (1) Nuevo `OdomChartView` (subclase de `QChartView`, archivo propio agregado a `BalancinQT.pro`): rueda del mouse = zoom centrado en el cursor (calcula un `QRectF` proporcional a la posición del mouse dentro del `plotArea()` y llama a `chart()->zoomIn(rect)`), arrastre con botón izquierdo = paneo (`chart()->scroll(-dx, dy)`), doble click = `zoomReset()`; emite señal `viewChanged()` tras cada interacción. (2) **Cinta ancha y negra**: se eliminaron los scatter verde/rojo por muestra; ahora mientras `line_detected` es verdadero se va extendiendo un `QLineSeries` grueso (`pen width=6`, `RoundCap`) — al perderse la línea el segmento se cierra (`odomTapeCurrent=nullptr`) y la próxima detección arranca un segmento nuevo (`QList<QLineSeries*> odomTapeSegments`), para que tramos de cinta separados por tramos sin línea no queden unidos por una recta. Solo el primer segmento aparece en la leyenda (los demás se ocultan con `QLegendMarker::setVisible(false)`). (3) **Anotaciones de eventos**: se detectan transiciones (no cada muestra) en `updateOdomChart()` — línea detectada→no detectada dispara "Línea perdida" (marcador rojo + texto), y `line_state` cruzando de `<13` a `>=13` (primer valor de la secuencia de evasión de objeto, `LINE_STATE_OBJ_ESPERA_REVERSA` en el firmware — frágil, documentado con comentario si se reordena el enum) dispara "Objeto" (marcador naranja + texto). El texto es un `QGraphicsSimpleTextItem` agregado directo a `odomChart->scene()` (no es parte de ninguna serie), reposicionado en cada pan/zoom/nuevo dato vía `repositionOdomAnnotations()` (usa `chart()->mapToPosition()`) — conectado a la señal `viewChanged()` de `OdomChartView`. (4) Botón "Reset vista" agregado sobre el mapa como alternativa al doble click. `updateOdomChart()` cambió de firma: ahora recibe también `lineState` (antes solo x/y/theta/lineDetected). | Usuario pidió mejorar el gráfico XY recién agregado: que sea un mapa navegable (zoom/paneo), que la cinta se dibuje ancha y negra en vez de puntitos, y que vayan quedando anotaciones donde se perdió la línea o se detectaron objetos |
| 2026-07-06 | mainwindow.h, mainwindow.cpp | **Handlers de odometría + gráfico XY**: (1) enum `_eCmd`: `GET_ODOMETRY=0xDA`/`RESET_ODOMETRY=0xDB`/`WIFI_ODOM_DATA=0xDC`. (2) `GET_ODOMETRY`/`RESET_ODOMETRY` agregados como comandos sin payload en los switches de envío (UDP y Serie) + 2 entradas nuevas en `comboBox_CMD`; respuesta logueada en `textEdit_PROCCES`. (3) Struct mirror `WifiOdomData_t` (`#pragma pack(1)`, mismo patrón que `WifiLogData_t`) — debe mantenerse en sincro con `Core/Inc/UNER.h` del STM32 si cambia el layout. (4) Nueva pestaña "Odometría (WiFi)" en `tabWidget_Graficas` (creada por código en el constructor, no se tocó el `.ui`): `QChart` con trayectoria (x,y) en gris, puntos verdes/rojos según si se veía la línea, marcador azul de posición actual, autoescala de ejes con margen 0.3m que solo crece, y label con X/Y/Theta/línea numéricos. Nuevo método `updateOdomChart()` y case `WIFI_ODOM_DATA` en `decodeData()` (parsing vía `reinterpret_cast`, igual que `WIFI_LOG_DATA`). El STM32 empuja este paquete solo (push, sin pedirlo) cada 500ms mientras haya WiFi conectado — no requiere activar `ACTIVATE_WIFI_LOG` ni ninguna acción desde Qt. | Usuario pidió cerrar la dependencia pendiente de odometría y agregar visualización gráfica (mapa XY + posición de línea) alimentada por WiFi a bajo ritmo para no competir con la telemetría de control |

---

## Decisiones de Diseño
> Registrar el *por qué* de decisiones importantes para no revertirlas por error.

- **Mismo protocolo UNER en Serial y UDP:** simplifica el firmware STM32, que puede usar el mismo parser en ambos canales sin bifurcar lógica.
- **Auto-descubrimiento de IP/puerto UDP:** el STM32 envía el primer paquete y Qt aprende la dirección del sender — evita configurar IP manualmente en la app.
- **Telemetría CSV por Serial / telemetría binaria por UDP:** CSV facilita debug directo con un terminal serie; struct binaria UDP es más eficiente a alta frecuencia.
- **Estilo Fusion de Qt forzado en `main.cpp`:** el estilo nativo de Windows ignora parcialmente QStyleSheet en QSplitter, QScrollBar, etc. Fusion lo respeta al 100%.
- **Qt3D para el visor 3D:** permite cargar el modelo OBJ directamente de Fusion 360 sin conversión, y la rotación se mapea directo al roll_filt del IMU.
- **Todo en mainwindow.cpp:** no hay clases separadas para serial/UDP/plots — decisión de simplicidad para un proyecto académico. Empezó a matizarse el 2026-07-06: `OdomChartView`/`HealthDashboard` se separaron a archivos propios por tener estado y comportamiento propio (no por una regla estricta) — el protocolo/parsing sigue centralizado en `mainwindow.cpp` a propósito.

---

## Dependencias con el Firmware STM32
> Cambios en Qt que requieren cambios **coordinados** en el firmware:

- Si modificás el **formato de trama** → actualizar el parser en `Core/Src/UNER.c` del STM32 (no existe `communication.c`; el protocolo binario UNER vive en `UNER.c`/`UNER.h`)
- Si agregás **parámetros nuevos** → sincronizar con el receptor de comandos en el STM32
- Si cambiás **puertos UDP** → actualizar configuración de red en el firmware
- Consultar siempre: `C:\Users\tadeo\STM32CubeIDE\workspace_1.18.1\Balancin_Mendelevich\CLAUDE.md`

---

## Comandos Útiles
```powershell
# Abrir el proyecto
cd "C:\Microcontroladores\BalancinQT"

# Ver puertos seriales disponibles
Get-WMIObject Win32_SerialPort | Select Name, DeviceID, Description

# Compilar desde consola (si usás qmake)
qmake BalancinQT.pro && nmake

# Compilar con CMake
cmake --build build --config Release
```
