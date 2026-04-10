# Comunicador Horus v4 — Macromedidor (M5Stack)

Proyecto **Horus Smart Energy** para equipos **EASTRON SDM630MCT V2**. Es el programa que lleva la **M5Stack**: hace de puente entre los **macromedidores** (por cable RS485) y el **servidor en la nube**, para que la energía que miden quede disponible en la plataforma.

En pantalla verás identificación tipo **Comunicador versión 4** (referencia Feb 2026).

## ¿Qué hace este equipo?

1. **Se conecta a internet** por WiFi o por cable Ethernet (LAN), según lo que configures.
2. **Lee los medidores** conectados en bus RS485 (hasta **tres** medidores en este firmware; direcciones de bus **01, 02 y 03** según la cantidad elegida).
3. **Guarda la configuración** en la memoria interna del ESP32 (no usa EEPROM clásica): red, proyecto, seriales, etc.
4. **Envía periódicamente** las mediciones al backend de Horus. Por defecto el firmware apunta al entorno de **pruebas** (`macrotest.horussmartenergyapp.com`); para producción hay que cambiar servidor y credenciales en el código (ver más abajo).
5. **Comprueba** si cada medidor está dado de alta en base de datos y te lo muestra en la pantalla (por ejemplo “Ok_Reg” o “No_Reg”).
6. **Mantiene la hora** sincronizada por internet. **Sin hora válida no se programan los envíos periódicos** de mediciones.

**Frecuencia de lectura y envío:** en operación normal los datos se leen y se intentan subir **cada 5 minutos** (alineado a minutos terminados en 0 o 5).

En resumen: cableas los medidores, configuras red y proyecto una vez, y el equipo **solo** sigue leyendo y subiendo datos.

## Requisitos de hardware (resumen)

- Placa **M5Stack** compatible con este firmware (ESP32).
- Si usas **Ethernet**, el módulo **LAN** de M5Stack (cable al router o switch).
- Bus **RS485** correctamente cableado hacia los macromedidores (mismo estándar que el medidor; revisar polaridad y, si aplica, terminación según instalación).
- Velocidad de puerto serie del bus en firmware: **9600 baud** (ajustes de pines avanzados en `index/lib/defines.h`).

## Puesta en marcha sugerida

1. Montar alimentación y cable de red o cobertura WiFi estable.
2. Conectar el bus RS485 a los medidores.
3. Encender y seguir el **menú por Serial** (USB) o la **configuración web** (ver siguiente apartado).
4. Comprobar en pantalla que aparecen **seriales** y estado **Ok_Reg** cuando el medidor ya existe en la plataforma.
5. Si algo falla, revisar la sección **Qué significan los mensajes** y el **diagnóstico de red** en pantalla.

## Botones al encender (M5Stack)

- **Botón B** mantenido al arrancar: entra en modo **configuración solo por web** (punto de acceso WiFi del equipo + página). Si la memoria ya tiene seriales y la red está completa, este modo puede mostrar **“config web bloqueada”**; entonces hay que borrar memoria o usar el menú por Serial.
- **Botón C** mantenido al arrancar: **borra toda la configuración** guardada en el equipo y **reinicia** (útil para dejar el comunicador como salido de fábrica).

En el menú por Serial también existe la opción de **borrar memoria** sin usar el botón C.

## Configuración por WiFi del propio equipo (portal web)

Cuando el equipo está en modo configuración web, con el celular o la PC:

1. Conéctate a la red WiFi del comunicador:
   - **Nombre (SSID):** indicada en pantalla
   - **Contraseña:** indicada en pantalla
2. Abre en el navegador:
   - **http://macromedhorus.com** (redirige al equipo), 
               o
   - **http://192.168.4.1**

Completa el formulario (WiFi o LAN, código de proyecto, cantidad de medidores y, si aplica, SSID y contraseña de tu casa/empresa). Al guardar, el equipo continúa el flujo (conexión a internet y lectura de seriales por RS485).

## Cómo se configura (idea general)

- **Menú por pantalla y puerto USB (Serial):** eliges WiFi o LAN, introduces datos de red, código de proyecto y cantidad de medidores; luego el equipo intenta leer los **números de serie** de cada medidor por el bus.
- **Configuración por celular o PC:** como arriba, con **botón B** al arranque cuando corresponda.
- **Borrar todo:** menú opción 3, o **botón C** al encender.

Si la memoria ya tiene seriales y red completa, el modo “solo web” puede quedar **bloqueado por seguridad**; en ese caso lo indica la pantalla: usar borrado o el menú clásico.

## Qué verás en la pantalla

- **Estado de red** (conectado o no, IP).
- **Seriales** de cada medidor y si están registrados en el sistema: **Ok_Reg**, **No_Reg** o **Sin_Resp** (sin respuesta o error al consultar).
- **Fecha y hora** cuando la sincronización fue correcta.
- Al enviar datos: **OK**, **ERR_Macro** (sin datos útiles del medidor) o **ERR_RED** (fallo de red o del servidor).

También puede mostrarse un **diagnóstico rápido** tras conectar (internet genérico, hora por red, respuesta del servidor). Sirve cuando un **firewall** de la empresa bloquea NTP, HTTPS u otros destinos: suele hacer falta que IT permita salida a internet, sincronización de hora y el dominio del backend Horus.

## Qué significan los mensajes (para soporte en campo)

| Lo que ves | Interpretación práctica |
|------------|-------------------------|
| **No_Reg** | El serial se leyó bien, pero **falta dar de alta** ese medidor en la plataforma o proyecto correcto. |
| **Sin_Resp** | No se pudo obtener estado desde el servidor (red, cortafuegos o respuesta inesperada). |
| **Ok_Reg** | El medidor figura registrado en base de datos para el flujo actual. |
| **ERR_Macro** al enviar | El bus no devolvió datos válidos para ese ciclo (cable, dirección, medidor apagado, etc.). |
| **ERR_RED** al enviar | Problema al llegar al servidor (WiFi, cable Ethernet, DNS, firewall). |

## Código y compilación

Todo el firmware está en la carpeta **`index/`**: ábrela como proyecto en **Arduino IDE** (varios archivos `.ino` que Arduino junta solo).

### Para desarrolladores (breve)

- Librerías típicas: **M5Stack**, **ArduinoJson**, **NTPClient**, **EthernetLarge**, además de WiFi, HTTPClient, WebServer, DNSServer y Preferences.
- Rutas REST listadas en **`index/_rutas.txt`**.
- Pines RS485, SPI Ethernet, flags de paridad RS485 y URL del API: **`index/lib/defines.h`**.

Los detalles de nombres de archivos fuente y lógica interna están en el propio código para mantenimiento y auditoría.
