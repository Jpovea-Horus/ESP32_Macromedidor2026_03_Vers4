# Macromedidor ESP32 v2 (queryModbus_2)

Proyecto desde cero usando **solo Preferences (NVS)** para persistencia. Sin EEPROM.

## Estructura

- `index/` — Sketch Arduino (abrir la carpeta `index` en Arduino IDE o PlatformIO).
- `index/index.ino` — setup/loop, flujo principal.
- `index/app_preferences.ino` — lectura/escritura NVS (namespace `macromed`).
- `index/config.ino` — menú configuración (pendiente completar).
- `index/network.ino` — WiFi/Ethernet unificado.
- `index/modbus.ino` — consultas Modbus (stub).
- `index/api.ino` — HTTP al backend (stub).
- `index/ui.ino` — pantalla M5.
- `index/lib/defines.h` — constantes, rutas API, claves Preferences.
- `index/lib/CRC.h`, `tramasModbus.h` — Modbus.

## Mejoras vs v1

- Persistencia solo con Preferences.
- Menos variables globales; claves NVS definidas en `defines.h`.
- Sin auto-creación de registros de comunicador (según backend).
- Si no hay Ethernet/WiFi, se puede continuar registro y guardar en NVS.

## Rutas API

Ver `index/_rutas.txt`. Mismo backend que v1 (macrotest.horussmartenergyapp.com).

## Cómo compilar    

1. Arduino IDE: abrir `queryModbus_2/index` como sketch; placa ESP32, librerías M5Stack, ArduinoJson, etc.
2. O crear `platformio.ini` en `queryModbus_2` con board `m5stack-core-esp32` y dependencias.