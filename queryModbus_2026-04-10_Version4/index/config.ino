/*
 * Configuración inicial: menú por Serial + LCD (logo y tamaño letra como v1).
 * 1. Red WiFi  2. Red LAN  3. Borrar Preferences
 */
#include "lib/defines.h"
#include <esp_system.h>
#include "image/xbm.h"

int apiGetSerialStatus(const String& serial);  // network.ino

// Lee un dígito (1, 2 o 3) por Serial para el menú inicial
static int configMenuReadDigit() {
  String inString = "";
  while (true) {
    while (Serial.available() > 0) {
      char c = Serial.read();
      if (isDigit(c)) inString += c;
    }
    if (!inString.isEmpty()) {
      int x = inString.toInt();
      if (x >= 1 && x <= 3) return x;
    }
    delay(50);
  }
}

// Lee una línea por Serial (para SSID, password, etc.)
static String configMenuReadLine() {
  while (Serial.available()) Serial.read();
  String s = "";
  while (true) {
    while (Serial.available() > 0) s += (char)Serial.read();
    if (!s.isEmpty()) break;
    delay(50);
  }
  s.trim();
  return s;
}

static void configMenuWifi() {
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("##SSID de tu red:");
  Serial.print("##SSID de tu red: ");
  String ssid = configMenuReadLine();
  prefsPutSsid(ssid.c_str());
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.println(ssid);
  Serial.println(ssid);

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.print("##password: ");
  Serial.print("##password: ");
  String pass = configMenuReadLine();
  prefsPutPass(pass.c_str());
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.println("********");
  Serial.println("********");
  delay(2000);
}

// Pide código de proyecto y cantidad de macromedidores por Serial (tras conexión WiFi/LAN).
void configMenuProjectAndMeters() {
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.println("Codigo de proyecto");
  Serial.print("#Ingrese codigo del proyecto: ");
  String key = configMenuReadLine();
  int projectId = key.toInt();
  prefsPutProjectId(projectId);
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.println(projectId);
  Serial.println(projectId);
  delay(1000);

  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.println("Cant. macromedidores (1-3)");
  Serial.println("#Ingrese la cantidad de medidores: ");
  key = configMenuReadLine();
  key.trim();
  int n = key.toInt();
  if (n < 1) n = 1;
  if (n > 3) n = 3;
  prefsPutMeterCount((uint8_t)n);

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Direccion medidores:");
  Serial.print("##Direccion de medidores: ");
  M5.Lcd.setTextColor(TFT_GREEN);
  if (n >= 1) {
    prefsPutMeterId(0, "01");
    M5.Lcd.print("01");
    Serial.print("01");
  }
  if (n >= 2) {
    prefsPutMeterId(1, "02");
    M5.Lcd.print(", 02");
    Serial.print(", 02");
  }
  if (n >= 3) {
    prefsPutMeterId(2, "03");
    M5.Lcd.print(", 03");
    Serial.print(", 03");
  }
  M5.Lcd.println();
  Serial.println();
  delay(2000);

  // Iniciar RS485 y preguntar a los macromedidores el serial
  configDoSerialsAndFinish();
}

// Solo obtiene seriales por RS485, registra y muestra pantalla listo (prefs ya tiene projectId, meterN, meterId).
void configDoSerialsAndFinish() {
  uint8_t n = prefsGetMeterCount();
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Obteniendo seriales");
  M5.Lcd.println("por RS485...");
  Serial.println("Obteniendo seriales por RS485...");
  delay(1500);
  if (!obtenerSerialesMedidoresRS485()) {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("Serial no encontrado.");
    M5.Lcd.println("Revisar conexion");
    M5.Lcd.println("con macromedidor.");
    Serial.println("Serial de medidores no encontrado. Revisar conexion RS485.");
    delay(5000);
  } else {
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.println("Seriales leidos OK");
    Serial.println("Seriales leidos OK");
    for (uint8_t k = 0; k < n; k++) {
      String s = prefsGetSerialMedidor(k);
      if (s.length() > 0) {
        M5.Lcd.print("M");
        M5.Lcd.print(k + 1);
        M5.Lcd.print(": ");
        M5.Lcd.println(s);
        Serial.println(s);
      }
    }
    M5.Lcd.println("Consultando BD...");
    Serial.println("Consultando estado en BD...");
    bool allRegistered = true;
    for (uint8_t k = 0; k < n; k++) {
      String s = prefsGetSerialMedidor(k);
      int st = (s.length() > 0) ? apiGetSerialStatus(s) : -1;
      prefsPutMeterRegStatus(k, st);
      if (st != 1) allRegistered = false;
      delay(300);
    }
    prefsPutRegistered(allRegistered);
    syncTimeFromNetwork();
    uiShowConfigDone();
    delay(3000);
  }
}

// Entrada solo por web: B al arranque + memoria vacía. AP + pantalla QR, esperar POST, conectar y seriales.
void configRunWebOnly() {
  webConfigStart();
  uiShowConfigQR();
  while (!webConfigReceivedFlag()) {
    webConfigLoop();
    delay(10);
  }
  webConfigStop();
  String mode = prefs.getString(PREF_KEY_MODE, "1");
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Config guardada.");
  M5.Lcd.println("Conectando...");
  if (mode == "1") {
    if (!networkConnectWiFi()) {
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.println("Revisar SSID/pass.");
      Serial.println("WiFi fallo.");
      delay(5000);
      return;
    }
  } else {
    if (!networkConnectLAN()) {
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.println("Revisar cable LAN.");
      Serial.println("LAN fallo.");
      delay(5000);
      return;
    }
  }
  configDoSerialsAndFinish();
}

void configRun() {
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Config preferences:");
  Serial.println("Config preferences:");
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.println("1. Red WiFi");
  Serial.println("1. Red WiFi");
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.println("2. Red LAN");
  Serial.println("2. Red LAN");
  M5.Lcd.setTextColor(TFT_ORANGE);
  M5.Lcd.println("3. Borrar Preferences");
  Serial.println("3. Borrar Preferences");

  M5.Lcd.drawXBitmap(LOGO_AXIS_X, LOGO_AXIS_Y, logo, logoWidth, logoHeight, TFT_WHITE, TFT_BLACK);

  int opcion = 0;
  while (true) {
    if (Serial.available()) {
      char c = (char)Serial.read();
      if (c >= '1' && c <= '3') {
        opcion = c - '0';
        Serial.print("Opcion: ");
        Serial.println(opcion);
        break;
      }
    }
    delay(10);
  }

  if (opcion == 1) {
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("1. Red WiFi: #");
    Serial.println("1. Red WiFi: #");
    prefsPutMode("1");
    configMenuWifi();
    prefsPutRegistered(false);
    prefsPutMeterCount(0);
    if (!networkConnectWiFi()) {
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.println("Revisar SSID y password.");
      Serial.println("WiFi fallo. Revisar SSID y password.");
      delay(5000);
    } else {
      configMenuProjectAndMeters();
    }
  } else if (opcion == 2) {
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("2. Red LAN: #");
    Serial.println("2. Red LAN: #");
    prefsPutMode("0");
    prefsPutRegistered(false);
    prefsPutMeterCount(0);
    if (!networkConnectLAN()) {
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.println("Revisar cable y reiniciar.");
      Serial.println("LAN fallo. Revisar cable y reiniciar.");
      delay(5000);
    } else {
      configMenuProjectAndMeters();
    }
  } else {
    // Opción 3: Borrar toda la memoria (Preferences)
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("3. Borrar memoria");
    Serial.println("3. Borrar Preferences: #");
    M5.Lcd.println("Borrando...");
    Serial.println("Borrando Preferences (NVS)...");

    WiFi.disconnect(true);  // Desconectar WiFi y borrar credenciales en RAM
    delay(200);

    preferencesWipe();  // Borra todo el namespace "macromed"

    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.println("Memoria borrada.");
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.println("Reiniciando...");
    Serial.println("Memoria borrada. Reiniciando comunicador.");
    delay(3000);
    esp_restart();             // Reinicio forzado; no retorna
    for (;;) { delay(1000); }  // Por si acaso
  }
}
