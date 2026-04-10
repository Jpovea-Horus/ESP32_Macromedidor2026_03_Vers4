#ifndef DEFINES_H
#define DEFINES_H

#include "WString.h"

#define SIZE 250
#define MAX_METERS 4

// Pines RS485: en tu placa LAN los cables van a 16 y 17 (no 5/15 del doc genérico)
#define SCK  18
#define MISO 19
#define MOSI 23
#define CS   26
#define RXPIN 16
#define TXPIN 17
#define DE 32
#define RE 33

#define BAUD 9600
#define TEST_UART 2
// RS485: 0 = 8N1 (como v1 Funcional parity=1, M5Stack LAN solo 16/17). 1 = 8E1
#define RS485_8E1 0
// 0 = no DE/RE (M5Stack diagrama solo 16/17, auto-direction en placa). 1 = controlar 32,33
#define RS485_USE_DE_RE 0

// Posición del logo en pantalla (como v1)
#define LOGO_AXIS_X 110
#define LOGO_AXIS_Y 140

// API (mismo backend que v1)
const char* PATH_PLOTS         = "/api/v1/meterhistory/plots/";
const char* PATH_COMMUNICATOR  = "/api/v1/Communicator/";
const char* PATH_REGISTERED    = "/api/v1/meter/";
const char* PATH_ERROR_METER   = "/api/v1/meterhistory/errorMeter/";
const char* PATH_SERIAL_STATUS = "/api/v1/meter/serial/";

const char* SERVER_URL = "https://www.macrotest.horussmartenergyapp.com";
const char* SERVER_HOST = "www.macrotest.horussmartenergyapp.com";
const int   SERVER_PORT = 80;
const char* API_KEY    = "test";  // apiKey (WiFi) / Bearer (LAN), como v1

// NTP y hora
const char* NTP_SERVER = "0.pool.ntp.org";
const long   GMT_OFFSET_SEC = 36000;
const int    DAYLIGHT_OFFSET_SEC = 60000;
#define EPOCH_MIN_2026        1735689600   // 1 ene 2026 00:00 UTC (validación mínima)
#define TIME_OFFSET_COL_SEC   (-18000)     // Colombia UTC-5 para NTP/HTTP

// Namespaces y claves Preferences (NVS) — única persistencia en v2
#define PREF_NAMESPACE "macromed"
#define PREF_KEY_MODE       "mode"        // "0" LAN, "1" WiFi
#define PREF_KEY_SSID       "ssid"
#define PREF_KEY_PASS       "pass"
#define PREF_KEY_REGISTERED "registered"  // "true" / "false"
#define PREF_KEY_METER_N    "meterN"      // cantidad
#define PREF_KEY_METER_ID   "meterId"     // meterId0, meterId1, ...
#define PREF_KEY_MAC        "mac"
#define PREF_KEY_DIR        "dir"
#define PREF_KEY_PROJECT_ID "projectId"
#define PREF_KEY_SERIAL_M   "serialM"   // serialM0, serialM1, serialM2 (serial del medidor)
#define PREF_KEY_METER_REG  "meterReg"  // meterReg0, meterReg1, ... 1=Ok_Reg, 0=No_Reg, -1=Sin_Resp

// Resultado en pantalla 3 (envío al servidor)
#define SEND_RESULT_OK        0
#define SEND_RESULT_ERR_MACRO 1   // Sin datos del macromedidor
#define SEND_RESULT_ERR_RED   2   // Error red/servidor

#endif
