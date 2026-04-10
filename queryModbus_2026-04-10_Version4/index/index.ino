/*
 * Macromedidor M5Stack v2 - Proyecto modbus -> server
 */
#include <M5Stack.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>

#include "lib/defines.h"
#include "lib/CRC.h"
#include "image/xbm.h"
#include <esp_system.h>

Preferences prefs;  // definido aquí; app_preferences.ino lo usa

// Estado global mínimo (reducir vs v1)
bool g_registered = false;
uint8_t g_meterCount = 0;
String g_meterIds[MAX_METERS];

void increaseWatchdogTime() {
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 10000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
}

void setup() {
  M5.begin();
  M5.Power.begin();
  Serial.begin(115200);
  M5.Lcd.setTextSize(2);
  delay(80);
  M5.update();
  bool bootWithB = M5.BtnB.isPressed();  // B al arranque: config web oculta (si memoria permite)
  // Si al reiniciar el botón C está presionado: borrar memoria (sin necesidad de Serial)
  if (M5.BtnC.isPressed()) {
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("Boton secreto: ON");
    M5.Lcd.println(" ... Borrando memoria ...");
    prefs.begin(PREF_NAMESPACE, false);
    preferencesWipe();
    WiFi.disconnect(true);
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.println("Memoria borrada.");
    M5.Lcd.println("Reiniciando...");
    delay(2000);
esp_restart();
  }
  increaseWatchdogTime();

  preferencesLoad();
  g_registered = prefsGetRegistered();
  g_meterCount = prefsGetMeterCount();
  for (uint8_t i = 0; i < g_meterCount && i < MAX_METERS; i++) {
    g_meterIds[i] = prefsGetMeterId(i);
  }
  prefs.end();
  prefs.begin(PREF_NAMESPACE, false);  // rw para config y resto

  uiShowSplash();

  // Flujo deseado:
  // - Boton B al arranque: config solo por web (oculta), salvo si ya hay datos completos en NVS.
  // - Sin boton B: config normal (menu WiFi/LAN por Serial/LCD).
  bool hasSerials = prefsHasSerialsStored();
  bool webBlocked = hasSerials && !prefsNeedsConfig();
  if (bootWithB) {
    if (webBlocked) {
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_YELLOW);
      M5.Lcd.println("Config web");
      M5.Lcd.println("bloqueada.");
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.println("Memoria OK.");
      M5.Lcd.println("Borra con C o");
      M5.Lcd.println("menu Serial.");
      Serial.println("Config web oculta bloqueada: NVS con seriales y red OK.");
      delay(3500);
    } else {
      configRunWebOnly();
      preferencesSave();
      g_registered = prefsGetRegistered();
      g_meterCount = prefsGetMeterCount();
    }
  } else if (!hasSerials || !g_registered || prefsNeedsConfig()) {
    configRun();
    preferencesSave();
    g_registered = prefsGetRegistered();
    g_meterCount = prefsGetMeterCount();
  }

  if (g_registered && g_meterCount > 0) {
    networkEnsureConnected();  // network.ino
    syncTimeFromNetwork();     // ntp.ino: NTP + respaldo HTTP
    networkDiagnoseConnectivity();  // Google, NTP, servidor — identifica tipo de bloqueo
    uiShowConfigDone();        // Pantalla: red, seriales, fecha/hora, logo
  }
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    // Opción: reset config (borrar Preferences)
    // preferencesWipe();
  }

  if (g_registered && g_meterCount > 0) {
    if (networkIsLinkUp()) {
      queryMetersLoop();  // Solo con red: cada 5 min lee 34 datos (para enviar al servidor)
    }

    // Consulta estado de red cada 60 s; si pasó de desconectado a conectado, re-adquirir sin reiniciar
    const unsigned long NETWORK_CHECK_INTERVAL_MS = 60000;
    static unsigned long lastNetworkCheck = 0;
    static bool lastLinkUp = false;
    static unsigned long firstNoNetworkMs = 0;  // 0 = hay red o aun no contamos
    bool needInit = (lastNetworkCheck == 0);
    unsigned long nowMs = millis();
    if (nowMs - lastNetworkCheck >= NETWORK_CHECK_INTERVAL_MS) {
      lastNetworkCheck = nowMs;
      bool linkUp = networkIsLinkUp();
      if (needInit) {
        lastLinkUp = linkUp;
      } else if (!lastLinkUp && linkUp) {
        Serial.println("Red detectada: re-adquiriendo conexion y hora...");
        networkEnsureConnected();
        syncTimeFromNetwork();
        Serial.println("Reconexion lista.");
        lastLinkUp = true;
      } else {
        lastLinkUp = linkUp;
      }
      // Si sin red >= 5 min, reiniciar para forzar recuperacion
      const unsigned long NO_NETWORK_REBOOT_MS = 5 * 60 * 1000;
      if (!linkUp) {
        if (firstNoNetworkMs == 0)
          firstNoNetworkMs = nowMs;
        else if ((nowMs - firstNoNetworkMs) >= NO_NETWORK_REBOOT_MS) {
          Serial.println("Sin red 5 min -> reinicio");
          delay(500);
          esp_restart();
        }
      } else {
        firstNoNetworkMs = 0;
      }
      uiShowConfigDone();
    }
    delay(100);
  } else {
    delay(500);
  }
}
